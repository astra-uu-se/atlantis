#include "atlantis/fznBackend.hpp"

#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include <fznparser/parser.hpp>

namespace atlantis {

FznBackend::FznBackend(
    std::filesystem::path&& modelFile,
    search::AnnealingScheduleFactory&& annealingScheduleFactory,
    std::uint_fast32_t seed, std::optional<std::chrono::milliseconds> timeout)
    : _modelFile(std::move(modelFile)),
      _annealingScheduleFactory(std::move(annealingScheduleFactory)),
      _timeout(timeout),
      _seed(seed) {}

FznBackend::FznBackend(
    std::filesystem::path&& modelFile,
    search::AnnealingScheduleFactory&& annealingScheduleFactory,
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
    logger.debug("Found {:d} variables", m.vars().size());
    logger.debug("Found {:d} constraints", m.constraints().size());
    return m;
  });

  fznparser::ProblemType problemType = model.solveType().problemType();

  invariantgraph::FznInvariantGraph invariantGraph;
  logger.timed<void>("building invariant graph",
                     [&] { return invariantGraph.build(model); });

  propagation::Solver solver;
  invariantGraph.apply(solver);
  auto neighbourhood = invariantGraph.neighbourhood();
  neighbourhood.printNeighbourhood(logger);

  search::Objective searchObjective(solver, problemType);
  solver.open();
  auto violation = searchObjective.registerNode(
      invariantGraph.totalViolationVarId(), invariantGraph.objectiveVarId());
  solver.close();

  search::Assignment assignment(solver, violation,
                                invariantGraph.objectiveVarId(),
                                getObjectiveDirection(problemType));

  logger.debug("Using seed {}.", _seed);
  search::RandomProvider random(_seed);

  search::SearchProcedure search(random, assignment, neighbourhood,
                                 searchObjective);

  const auto& outputBoolVars = invariantGraph.outputBoolVars();
  const auto& outputIntVars = invariantGraph.outputIntVars();
  const auto& outputBoolVarArrays = invariantGraph.outputBoolVarArrays();
  const auto& outputIntVarArrays = invariantGraph.outputIntVarArrays();

  search::SearchController searchController = [&] {
    if (_timeout) {
      return search::SearchController(model, invariantGraph, *_timeout);
    } else {
      logger.info("Running without timeout.");
      return search::SearchController(model, invariantGraph);
    }
  }();

  auto schedule = _annealingScheduleFactory.create();
  search::Annealer annealer(assignment, random, *schedule);

  return logger.timed<search::SearchStatistics>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}

}  // namespace atlantis
