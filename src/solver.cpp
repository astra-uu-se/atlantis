#include "solver.hpp"

#include <iostream>

#include "fznparser/modelFactory.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "search/objective.hpp"
#include "search/searchProcedure.hpp"

Solver::Solver(std::filesystem::path modelFile,
               search::AnnealingScheduleFactory annealingScheduleFactory,
               std::chrono::milliseconds timeout)
    : Solver(std::move(modelFile), std::move(annealingScheduleFactory),
             std::time(nullptr), timeout) {}

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

search::SearchStatistics Solver::solve(logging::Logger& logger) {
  auto model = logger.timed<fznparser::FZNModel>("parsing FlatZinc", [&] {
    auto m = fznparser::ModelFactory::create(_modelFile);
    logger.debug("Found {:d} variables", m.variables().size());
    logger.debug("Found {:d} constraints", m.constraints().size());
    return m;
  });

  invariantgraph::InvariantGraphBuilder invariantGraphBuilder;
  auto graph = logger.timed<invariantgraph::InvariantGraph>(
      "building invariant graph",
      [&] { return invariantGraphBuilder.build(model); });

  PropagationEngine engine;
  auto applicationResult = graph.apply(engine);
  auto neighbourhood = applicationResult.neighbourhood();
  neighbourhood.printNeighbourhood(logger);

  search::Objective searchObjective(engine, model);
  engine.open();
  auto violation =
      searchObjective.registerWithEngine(applicationResult.totalViolations(),
                                         applicationResult.objectiveVariable());
  engine.close();

  search::Assignment assignment(engine, violation,
                                applicationResult.objectiveVariable(),
                                getObjectiveDirection(model.objective()));

  logger.debug("Using seed {}.", _seed);
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
      logger.info("Running without timeout.");
      return search::SearchController(model, flippedMap);
    }
  }();

  auto schedule = _annealingScheduleFactory.create();
  search::Annealer annealer(assignment, random, *schedule);

  return logger.timed<search::SearchStatistics>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}
