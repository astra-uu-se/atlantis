#include "atlantis/solverThread.hpp"

#include <fstream>
#include <fznparser/parser.hpp>
#include <utility>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
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

void SolverThread::solve(logging::Logger& logger) {
  // Create the propagation solver
  propagation::Solver solver;
  solver.open();

  invariantgraph::SolverMapping mapping = _invariantGraph->construct(solver);

  // Might be changed to shared later
  search::Objective searchObjective(solver, mapping.objectiveDirection());

  const auto violationId = searchObjective.registerNode(
      mapping.totalViolationId(), mapping.objectiveId());
  solver.close();

  // retrieve the variables that are to be outputted
  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(_outputVarNodeIds.size());
  for (const auto oId : _outputVarNodeIds) {
    outputVarIds.emplace_back(mapping.solverId(oId));
  }

  search::Assignment assignment(
      solver, mapping.globalNeighborhood(), violationId, mapping.objectiveId(),
      mapping.objectiveDirection(), mapping.objectiveOptimalValue());

  // TODO: This can possibly be extracted, or restricted to one thread
  // TODO: this case may not be handled properly
  if (mapping.globalNeighborhood()->coveredVars().empty()) {
    search::SavedAssignment savedAssignment(assignment, outputVarIds);
    _onSolution(savedAssignment, *_threadController, _threadId);
    _onFinish(true);
  }

  // Initialize thread-dependent stuff
  logger.debug("Thread {} Using seed {}.", _threadId, _seed);
  search::RandomProvider random(_seed);
  search::Annealer annealer(random, *_schedule, assignment);
  search::SearchProcedure search(
      random, assignment, mapping.globalNeighborhood(), searchObjective,
      _searchType, _threadController, outputVarIds, _threadId);

  auto onSolution = [&](const search::SavedAssignment& savedAssignment) {
    _onSolution(savedAssignment, *_threadController, _threadId);
  };
  auto onFinish = [&](const bool hadSol) { _onFinish(hadSol); };
  search::SearchController searchController(
      mapping.objectiveDirection() == ObjectiveDirection::NONE,
      std::move(onSolution), std::move(onFinish), _timelimit,
      *_threadController);

  logger.timedFunction<int>(
      "search", [&] { return search.run(searchController, annealer, logger); });

  _threadController->threadIsDone();
}

}  // namespace atlantis