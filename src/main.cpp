#include <chrono>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "misc/logging.hpp"
#include "solver.hpp"

/**
 * @brief Read a duration in milliseconds from an input stream. Used to allow
 * cxxopts to parse the duration for us.
 *
 * @param is The input stream to parse from.
 * @param duration The reference to the value that holds the duration.
 * @return std::istream& The modified input stream.
 */
std::istream& operator>>(std::istream& is, std::chrono::milliseconds& duration);

int main(int argc, char* argv[]) {
  try {
    // TODO: How do we want to control this? The log messages don't appear
    // in release builds, so do we still want a command line flag to set this?
    setLogLevel(debug);

    cxxopts::Options options(
        argv[0], "Constraint-based local search backend for MiniZinc.");

    options.positional_help("[flatzinc file]").show_positional_help();

    // Clang Format will make these option definitions completely garbled, hence
    // the comments to turn it off for this region.
    // clang-format off
    options.add_options()
      (
        "no-timeout",
        "Run the solver without a timeout. This means the solver needs to be stopped with a signal, which means no statistics will be printed."
      )
      (
        "t,time-limit",
        "Wall time limit in milliseconds.",
        cxxopts::value<std::chrono::milliseconds>()->default_value("30000") // 30 seconds
      )
      (
        "r,seed",
        "The seed to use for the random number generator. If this is negative, the current system time is chosen as the seed.",
        cxxopts::value<long>()->default_value("-1")
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

    auto& modelFilePath = result["modelFile"].as<std::filesystem::path>();

    auto givenSeed = result["seed"].as<long>();
    std::uint_fast32_t seed = givenSeed >= 0
                                  ? static_cast<std::uint_fast32_t>(givenSeed)
                                  : std::time(nullptr);

    std::optional<std::chrono::milliseconds> timeout = [&] {
      if (result.count("no-timeout") == 0) {
        return std::optional<std::chrono::milliseconds>{
            result["time-limit"].as<std::chrono::milliseconds>()};
      } else {
        logInfo("Running without timeout.");
        return std::optional<std::chrono::milliseconds>{};
      }
    }();

    Solver solver(modelFilePath, seed, timeout);
    auto statistics = solver.solve();

    // Don't log to std::cout, since that would interfere with MiniZinc.
    statistics.display(std::cerr);
  } catch (const cxxopts::OptionException& e) {
    std::cerr << "error: " << e.what() << std::endl;
  } catch (const std::invalid_argument& e) {
    std::cerr << "error: " << e.what() << std::endl;
  }
}

std::istream& operator>>(std::istream& is,
                         std::chrono::milliseconds& duration) {
  long x;
  auto& is2 = is >> x;

  duration = std::chrono::milliseconds(x);

  return is2;
}