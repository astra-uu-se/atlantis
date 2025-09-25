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

void SolverThread::solve(std::shared_ptr<const invariantgraph::FznInvariantGraph> invariantGraph, logging::Logger& logger) {
  propagation::Solver solver;

  solver.open();
  // TODO: we should improve the initialisation in order to avoid the need for
  // breaking the dynamic cycles
  invariantgraph::SolverMapping mapping = invariantGraph->construct(solver);

  // Might be changed to shared later
  search::Objective searchObjective(solver, _problemType);

  const auto violationId = searchObjective.registerNode(
      mapping.totalViolationId(), mapping.objectiveId());
  solver.close();

  // TODO: extract to shared -- requires the original invariantGraph
  const Int objectiveOptimalValue =
      _objectiveDirection == ObjectiveDirection::NONE ? 0
      : _objectiveDirection == ObjectiveDirection::MINIMIZE
          ? invariantGraph->objectiveVarNode().lowerBound()
          : invariantGraph->objectiveVarNode().upperBound();

  search::Assignment assignment(solver, mapping.globalNeighborhood(), violationId,
                                mapping.objectiveId(),
                                _objectiveDirection, objectiveOptimalValue);

  // This can possibly be extracted, or restricted to one thread
  // TODO: this case may not be handled properly
  if (mapping.globalNeighborhood()->coveredVars().empty()) {
    search::SavedAssignment savedAssignment =
        _onSolution(mapping, assignment, *_controller, _threadId);
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
    return _onSolution(mapping, a, *_controller, _threadId);
  };
  auto onFinish = [&](const bool hadSol) { _onFinish(hadSol); };
  search::SearchController searchController(_objectiveDirection == ObjectiveDirection::NONE,
                                            std::move(onSolution),
                                            std::move(onFinish), _timelimit);

  logger.timedFunction<int>(
      "search", [&] { return search.run(searchController, annealer, logger); });
}

}  // namespace atlantis