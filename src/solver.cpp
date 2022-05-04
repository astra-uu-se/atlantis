#include "solver.hpp"

#include "fznparser/modelFactory.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "search/objective.hpp"
#include "search/searchProcedure.hpp"
#include "search/annealing/annealerFacade.hpp"

Solver::Solver(std::filesystem::path modelFile,
               std::chrono::milliseconds timeout)
    : Solver(std::move(modelFile), std::time(nullptr), timeout) {}

static ObjectiveDirection getObjectiveDirection(
    const fznparser::Objective& objective) {
  return std::visit<ObjectiveDirection>(
      overloaded{
          [](const fznparser::Satisfy&) { return ObjectiveDirection::NONE; },
          [](const fznparser::Minimise&) {
            return ObjectiveDirection::MINIMISE;
          },
          [](const fznparser::Maximise&) {
            return ObjectiveDirection::MAXIMISE;
          }},
      objective);
}

search::SearchStatistics Solver::solve() {
  auto model = fznparser::ModelFactory::create(_modelFile);

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
  auto violation =
      searchObjective.registerWithEngine(applicationResult.totalViolations(),
                                         applicationResult.objectiveVariable());
  engine.close();

  search::Assignment assignment(engine, violation,
                                applicationResult.objectiveVariable(),
                                getObjectiveDirection(model.objective()));

  logDebug("Using seed " << _seed);
  search::RandomProvider random(_seed);

  search::SearchProcedure search(random, assignment, neighbourhood,
                                 searchObjective);

  search::SearchController::VariableMap flippedMap;
  for (const auto& [varId, fznVar] : applicationResult.variableMap())
    flippedMap.emplace(fznVar, varId);

  search::SearchController searchController = [&] {
    if (_timeout) {
      return search::SearchController(model, flippedMap, *_timeout);
    } else {
      logInfo("Running without timeout.");
      return search::SearchController(model, flippedMap);
    }
  }();

  auto schedule = search::AnnealerFacade::cooling(0.95, 0.001, 50);
  schedule->start(10.0);
  search::Annealer annealer(assignment, random, *schedule);
  return search.run(searchController, annealer);
}
