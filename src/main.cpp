#include <chrono>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "atlantis/fznBackend.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"

/**
 * @brief Read a duration in milliseconds from an input stream. Used to allow
 * cxxopts to parse the duration for us.
 *
 * @param is The input stream to parse from.
 * @param duration The reference to the value that holds the duration.
 * @return std::istream& The modified input stream.
 */
atlantis::logging::Level getLogLevel(cxxopts::ParseResult& result);

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options(
        argv[0], "Constraint-based local search backend for MiniZinc.");

    options.positional_help("[flatzinc file]").show_positional_help();

    // Clang Format will make these option definitions completely garbled, hence
    // the comments to turn it off for this region.
    // clang-format off
    options.add_options()
      (
        "i,intermediate-solutions",
        "Ignored, but present because used in the MiniZinc challenge."
      )
      (
        "t,time-limit",
        "Wall time limit in milliseconds.",
        cxxopts::value<long>()->default_value("30000") // 30 seconds
      )
      (
        "r,seed",
        "The seed to use for the random number generator. If this is negative, the current system time is chosen as the seed.",
        cxxopts::value<long>()->default_value("-1")
      )
      (
        "annealing-schedule",
        "A file path to the annealing schedule definition.",
        cxxopts::value<std::filesystem::path>()
      )
      (
        "log-level",
        "Configures the log level. 0 = ERROR, 1 = WARNING, 2 = INFO, 3 = DEBUG, 4 = TRACE. If not specified, the WARN level is used.",
        cxxopts::value<uint8_t>()
      )
      (
        "dot-file", "A file path where a dot file format of the invariant graph is to be saved.",
        cxxopts::value<std::filesystem::path>()
      )
      ("help", "Print help");

    options.add_options("Positional")
      (
        "modelFile",
        "Path to the flatzinc file which contains the model.",
        cxxopts::value<std::filesystem::path>()
      );

    // clang-format on

    options.parse_positional({"modelFile"});

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      return 0;
    }

    atlantis::logging::Logger logger(stderr, getLogLevel(result));

    auto modelFilePath = result["modelFile"].as<std::filesystem::path>();

    atlantis::FznBackend backend(logger, std::move(modelFilePath));

    auto givenSeed = result["seed"].as<long>();
    if (givenSeed >= 0) {
      backend.setRandomSeed(static_cast<std::uint_fast32_t>(givenSeed));
    }

    if (result.count("time-limit") == 1) {
      backend.setTimelimit(std::optional<std::chrono::milliseconds>{
          std::chrono::milliseconds(result["time-limit"].as<long>())});
    }

    if (result.count("annealing-schedule") == 1) {
      backend.setAnnealingScheduleFactory(
          atlantis::search::AnnealingScheduleFactory(
              result["annealing-schedule"].as<std::filesystem::path>()));
    }

    if (result.count("dot-file") == 1) {
      auto dotFilePath = result["dot-file"].as<std::filesystem::path>();
      backend.setDotFilePath(std::move(dotFilePath));
    }

    auto statistics = backend.solve(logger);

    // Don't log to std::cout, since that would interfere with MiniZinc.
    statistics.display(std::cerr);
  } catch (const cxxopts::OptionException& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  } catch (const std::invalid_argument& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}

atlantis::logging::Level getLogLevel(cxxopts::ParseResult& result) {
  if (result.count("log-level") != 1) {
    return atlantis::logging::Level::LVL_WARNING;
  }

  switch (result["log-level"].as<uint8_t>()) {
    case 0:
      return atlantis::logging::Level::LVL_ERROR;
    case 1:
      return atlantis::logging::Level::LVL_WARNING;
    case 2:
      return atlantis::logging::Level::LVL_INFO;
    case 3:
      return atlantis::logging::Level::LVL_DEBUG;
    case 4:
      return atlantis::logging::Level::LVL_TRACE;
    default:
      throw cxxopts::OptionException("The log level should be 0, 1, 2 or 3.");
  }
}
