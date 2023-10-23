#include <iostream>

#include "fznBackend.hpp"
#include "fznparser/parser.hpp"
#include "invariantgraph/invariantGraphBuilder.hpp"
#include "search/objective.hpp"
#include "search/searchProcedure.hpp"

namespace atlantis {

FznBackend::FznBackend(
    std::filesystem::path modelFile,
    search::AnnealingScheduleFactory annealingScheduleFactory,
    std::chrono::milliseconds timeout)
    : FznBackend(std::move(modelFile), std::move(annealingScheduleFactory),
                 std::time(nullptr), timeout) {}

static propagation::ObjectiveDirection getObjectiveDirection(
    fznparser::ProblemType problemType) {
  switch (problemType) {
    case fznparser::ProblemType::MINIMIZE:
      return propagation::ObjectiveDirection::MINIMIZE;
    case fznparser::ProblemType::MAXIMIZE:
      return propagation::ObjectiveDirection::MAXIMIZE;
    case fznparser::ProblemType::SATISFY:
    default:
      return propagation::ObjectiveDirection::NONE;
  }
}

search::SearchStatistics FznBackend::solve(logging::Logger& logger) {
  auto model = logger.timed<fznparser::Model>("parsing FlatZinc", [&] {
    auto m = fznparser::parseFznFile(_modelFile);
    logger.debug("Found {:d} variables", m.variables().size());
    logger.debug("Found {:d} constraints", m.constraints().size());
    return m;
  });

  fznparser::ProblemType problemType = model.solveType().problemType();

  invariantgraph::InvariantGraphBuilder invariantGraphBuilder(model);
  auto graph = logger.timed<invariantgraph::InvariantGraph>(
      "building invariant graph",
      [&] { return invariantGraphBuilder.build(); });

  propagation::Solver solver;
  auto applicationResult = graph.apply(solver);
  auto neighbourhood = applicationResult.neighbourhood();
  neighbourhood.printNeighbourhood(logger);

  search::Objective searchObjective(solver, problemType);
  solver.open();
  auto violation = searchObjective.registerNode(
      applicationResult.totalViolationId(), applicationResult.objectiveVarId());
  solver.close();

  search::Assignment assignment(solver, violation,
                                applicationResult.objectiveVarId(),
                                getObjectiveDirection(problemType));

  logger.debug("Using seed {}.", _seed);
  search::RandomProvider random(_seed);

  search::SearchProcedure search(random, assignment, neighbourhood,
                                 searchObjective);

  search::SearchController::VariableMap flippedMap;
  for (const auto& [varId, fznVar] : applicationResult.variableIdentifiers())
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

}  // namespace atlantis