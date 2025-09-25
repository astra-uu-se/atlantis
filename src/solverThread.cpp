#include "atlantis/solverThread.hpp"

#include <fstream>
#include <fznparser/parser.hpp>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include "atlantis/types.hpp"

namespace atlantis {

static ObjectiveDirection getObjectiveDirection(
    fznparser::ProblemType problemType) {
  switch (problemType) {
    case fznparser::ProblemType::MINIMIZE:
      return ObjectiveDirection::MINIMIZE;
    case fznparser::ProblemType::MAXIMIZE:
      return ObjectiveDirection::MAXIMIZE;
    case fznparser::ProblemType::SATISFY:
      default:
        return ObjectiveDirection::NONE;
  }
}

void SolverThread::solve(logging::Logger& logger) {
  propagation::Solver solver;
  solver.open();
  // TODO: we should improve the initialisation in order to avoid the need for
  // breaking the dynamic cycles
  invariantgraph::SolverMapping mapping = _invariantGraph->construct(solver);
  FznOutput fznOutput = _invariantGraph->generateFznOutput(mapping);

  // Might be changed to shared later
  search::Objective searchObjective(solver, _problemType);

  const auto violationId = searchObjective.registerNode(
      mapping.totalViolationId(), mapping.objectiveId());
  solver.close();

  const auto _objectiveDirection = getObjectiveDirection(_problemType);

  // TODO: extract to shared -- requires the original invariantGraph
  const Int objectiveOptimalValue =
      _objectiveDirection == ObjectiveDirection::NONE ? 0
      : _objectiveDirection == ObjectiveDirection::MINIMIZE
          ? _invariantGraph->objectiveVarNode().lowerBound()
          : _invariantGraph->objectiveVarNode().upperBound();

  search::Assignment assignment(solver, mapping.globalNeighborhood(), violationId,
                                mapping.objectiveId(),
                                _objectiveDirection, objectiveOptimalValue);

  // This can possibly be extracted, or restricted to one thread
  // TODO: this case may not be handled properly
  if (mapping.globalNeighborhood()->coveredVars().empty()) {
    search::SavedAssignment savedAssignment =
        _onSolution(assignment, fznOutput, *_controller, _threadId);
    _onFinish(true);
  }

  // Initialize thread-dependent stuff
  logger.debug("Thread {} Using seed {}.", _threadId, _seed);
  search::RandomProvider random(_seed);
  search::Annealer annealer(random, *_schedule, assignment);
  search::SearchProcedure search(random, assignment, mapping.globalNeighborhood(),
                                 searchObjective);

  // TODO: extract to shared -- requires fixing invariantGraph
  auto onSolution = [&](const search::Assignment& a) {
    return _onSolution(a, fznOutput, *_controller, _threadId);
  };
  auto onFinish = [&](const bool hadSol) { _onFinish(hadSol); };
  search::SearchController searchController(_objectiveDirection == ObjectiveDirection::NONE,
                                            std::move(onSolution),
                                            std::move(onFinish), _timelimit);

  logger.timedFunction<int>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}

}  // namespace atlantis