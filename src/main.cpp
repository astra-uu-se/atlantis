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

static search::Cost::ObjectiveDirection getObjectiveDirection(
    const fznparser::Objective& variant);

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
    auto model = fznparser::ModelFactory::create(modelFilePath);

    // Not used for output, but this allows running end-to-end tests with the
    // CLI on the structure identification.
    invariantgraph::InvariantGraphBuilder invariantGraphBuilder;
    auto graph = invariantGraphBuilder.build(model);

    PropagationEngine engine;
    auto applicationResult = graph.apply(engine);
    auto neighbourhood = applicationResult.neighbourhood();
    neighbourhood.printNeighbourhood(std::cerr);

    search::Objective searchObjective(engine, model);
    engine.open();
    auto violation = searchObjective.registerWithEngine(
        applicationResult.totalViolations(),
        applicationResult.objectiveVariable());
    engine.close();

    search::Assignment assignment(engine, violation,
                                  applicationResult.objectiveVariable(),
                                  getObjectiveDirection(model.objective()));

    auto givenSeed = result["seed"].as<long>();
    std::uint_fast32_t seed = givenSeed >= 0
                                  ? static_cast<std::uint_fast32_t>(givenSeed)
                                  : std::time(nullptr);
    logDebug("Using seed " << seed);
    search::RandomProvider random(seed);

    search::SearchProcedure search(random, assignment, neighbourhood,
                                   searchObjective);

    search::SearchController::VariableMap flippedMap;
    for (const auto& [varId, fznVar] : applicationResult.variableMap())
      flippedMap.emplace(fznVar, varId);

    search::SearchController searchController = [&] {
      if (result.count("no-timeout") == 0) {
        return search::SearchController(
            model, flippedMap,
            result["time-limit"].as<std::chrono::milliseconds>());
      } else {
        logInfo("Running without timeout.");
        return search::SearchController(model, flippedMap);
      }
    }();

    search::Annealer annealer(assignment, random);
    auto statistics = search.run(searchController, annealer);

    // Don't log to std::cout, since that would interfere with MiniZinc.
    statistics.display(std::cerr);
  } catch (const cxxopts::OptionException& e) {
    std::cerr << "error: " << e.what() << std::endl;
  } catch (const std::invalid_argument& e) {
    std::cerr << "error: " << e.what() << std::endl;
  }
}

static search::Cost::ObjectiveDirection getObjectiveDirection(
    const fznparser::Objective& objective) {
  return std::visit<search::Cost::ObjectiveDirection>(
      overloaded{[](const fznparser::Satisfy&) {
                   return search::Cost::ObjectiveDirection::NONE;
                 },
                 [](const fznparser::Minimise&) {
                   return search::Cost::ObjectiveDirection::MINIMISE;
                 },
                 [](const fznparser::Maximise&) {
                   return search::Cost::ObjectiveDirection::MAXIMISE;
                 }},
      objective);
}

std::istream& operator>>(std::istream& is,
                         std::chrono::milliseconds& duration) {
  long x;
  auto& is2 = is >> x;

  duration = std::chrono::milliseconds(x);

  return is2;
}