#include <chrono>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>

#include "core/propagationEngine.hpp"
#include "fznparser/modelFactory.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "misc/logging.hpp"
#include "search/assignment.hpp"
#include "search/neighbourhoods/randomNeighbourhood.hpp"
#include "search/searchProcedure.hpp"

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
        "n,num-solutions",
        "Instructs the solver to stop after reporting i solutions (only used with satisfaction problems).",
        cxxopts::value<uint>()->default_value("1")
      )
      (
        "t,time-limit",
        "Wall time limit in milliseconds.",
        cxxopts::value<std::chrono::milliseconds>()
      )
      (
        "s,seed",
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
    auto model = fznparser::ModelFactory::create(modelFilePath);

    // Not used for output, but this allows running end-to-end tests with the
    // CLI on the structure identification.
    invariantgraph::InvariantGraphBuilder invariantGraphBuilder;
    auto graph = invariantGraphBuilder.build(model);

    PropagationEngine engine;
    auto applicationResult = graph->apply(engine);

    search::Assignment assignment(engine, applicationResult.totalViolations(),
                                  applicationResult.objectiveVariable());

    auto givenSeed = result["seed"].as<long>();
    std::uint_fast32_t seed = givenSeed >= 0
                                  ? static_cast<std::uint_fast32_t>(givenSeed)
                                  : std::time(nullptr);
    logDebug("Using seed " << seed);
    search::RandomProvider random(seed);

    // TODO: Convert different types of neighbourhoods.
    auto neighbourhood =
        applicationResult.implicitConstraints().front()->takeNeighbourhood();
    search::SearchProcedure search(random, assignment, *neighbourhood);

    search::SolutionListener::VariableMap flippedMap;
    for (const auto& [varId, fznVar] : applicationResult.variableMap())
      flippedMap.emplace(fznVar, varId);

    search::SolutionListener solutionListener(*model, flippedMap);

    search::SearchController searchController;
    if (result.count("time-limit")) {
      searchController = search::SearchController(
          result["time-limit"].as<std::chrono::milliseconds>());
    }

    search::Annealer annealer(assignment, random);
    search.run(searchController, solutionListener, annealer);
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