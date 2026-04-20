#include <chrono>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "atlantis/fznBackend.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/searchStatistics.hpp"

atlantis::logging::Level getLogLevel(const cxxopts::ParseResult&);

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
      (
        "threads",
        "The number of threads to use for the search",
        cxxopts::value<std::uint_fast32_t>()->default_value("1")
      )
    (
      "communication-type",
      "The type of communication between the threads",
      cxxopts::value<std::uint_fast32_t>()->default_value("2")
    )
    (
      "bandit-algorithm",
      "The MAB annealing schedule selector algorithm",
      cxxopts::value<std::uint_fast32_t>()->default_value("0")
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

    const auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      return 0;
    }

    atlantis::logging::Logger logger(stderr, getLogLevel(result));

    auto modelFilePath = result["modelFile"].as<std::filesystem::path>();

    // TODO: this needs to be tested
    std::uint_fast32_t threadCount = 1;
    if (result.count("threads") == 1) {
      threadCount = result["threads"].as<uint_fast32_t>();
      if (threadCount < 1) {
        std::cout << "Error: Invalid thread count" << std::endl;
        return 2;
      }
    }

    atlantis::search::SearchType searchType =
        atlantis::search::SearchType::BEAMSEARCH;
    if (result.count("communication-type") == 1) {
      size_t searchTypeNumber =
          result["communication-type"].as<uint_fast32_t>();
      if (searchTypeNumber > 2) {
        std::cout << "Error: Invalid search type. Must be in the range 0-2."
                  << std::endl;
        return 0;
      }
      searchType = static_cast<atlantis::search::SearchType>(searchTypeNumber);
    }

    atlantis::search::BanditAlgorithm banditAlgorithm =
        atlantis::search::BanditAlgorithm::ETC;
    if (result.count("bandit-algorithm") == 1) {
      size_t algorithmNumber = result["bandit-algorithm"].as<uint_fast32_t>();
      if (algorithmNumber > 3) {
        std::cout
            << "Error: Invalid bandit algorithm. Must be in the range 0-3."
            << std::endl;
        return 0;
      }
      banditAlgorithm =
          static_cast<atlantis::search::BanditAlgorithm>(algorithmNumber);
    }

    atlantis::FznBackend backend(logger, std::move(modelFilePath), threadCount,
                                 searchType, banditAlgorithm);

    if (long givenSeed; (givenSeed = result["seed"].as<long>()) >= 0) {
      backend.setRandomSeed(static_cast<std::uint_fast32_t>(givenSeed));
    }

    if (result.count("time-limit") == 1) {
      backend.setTimelimit(std::optional{
          std::chrono::milliseconds(result["time-limit"].as<long>())});
    }

    if (result.count("annealing-schedule") == 1) {
      backend.setAnnealingScheduleFactory(
          std::make_shared<atlantis::search::AnnealingScheduleFactory>(
              result["annealing-schedule"].as<std::filesystem::path>()));
    }

    if (result.count("dot-file") == 1) {
      auto dotFilePath = result["dot-file"].as<std::filesystem::path>();
      backend.setDotFilePath(std::move(dotFilePath));
    }

    try {
      backend.solve(logger);
      backend.join(logger);
    } catch (const std::exception& e) {
      try {
        backend.join(logger);
      } catch (...) {
      }
      std::cerr << "Internal Atlantis error: " << e.what() << std::endl;
      return 1;
    } catch (...) {
      try {
        backend.join(logger);
      } catch (...) {
      }
      std::cerr << "Internal Atlantis error: unknown non-standard exception"
                << std::endl;
      return 1;
    }

    return 0;

  } catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 2;
  } catch (const std::invalid_argument& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 2;
  }
  return 1;
}

atlantis::logging::Level getLogLevel(const cxxopts::ParseResult& result) {
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
      throw cxxopts::exceptions::exception(
          "The log level should be 0, 1, 2 or 3.");
  }
}
