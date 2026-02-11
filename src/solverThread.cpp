#include "atlantis/solverThread.hpp"

#include <fstream>
#include <fznparser/parser.hpp>
#include <utility>

#include "atlantis/fznBackend.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/logging/logger.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealer.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/metaheuristic.hpp"
#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"
#include "atlantis/search/objective.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"
#include "atlantis/search/searchProcedure.hpp"
#include "atlantis/types.hpp"

namespace atlantis {

SolverThread::SolverThread(FznBackend& backend, size_t threadId)
    : SolverThread(backend.invariantGraph(), backend.outputVarNodeIds(),
                   backend.problemType(), backend.annealingScheduleFactory(),
                   threadId, backend.threadController(), backend.searchType(),
                   backend.onMove(), backend.seed(), backend.timelimit(),
                   backend.shouldStop()) {}

SolverThread::SolverThread(
    const std::shared_ptr<const invariantgraph::FznInvariantGraph>&
        invariantGraph,
    std::vector<invariantgraph::VarNodeId>&& outputVarNodeIds,
    const fznparser::ProblemType problemType,
    const std::shared_ptr<const search::AnnealingScheduleFactory>&
        annealingScheduleFactory,
    const size_t threadId,
    const std::shared_ptr<search::ThreadController>& controller,
    search::SearchType searchType,
    std::function<void(search::ThreadController&)>& onMove,
    const std::uint_fast32_t seed,
    const std::optional<std::chrono::milliseconds> timeLimit,
    const std::shared_ptr<const bool>& shouldStop)
    : _invariantGraph(invariantGraph),
      _outputVarNodeIds(std::move(outputVarNodeIds)),
      _annealingScheduleFactory(annealingScheduleFactory),
      _problemType(problemType),
      _threadId(threadId),
      _threadController(controller),
      _searchType(searchType),
      _onMove(onMove),
      _seed(seed + threadId),
      _timelimit(timeLimit),
      _shouldStop(shouldStop) {}

std::unique_ptr<search::MetaHeuristic> SolverThread::createMetaHeuristic(
    search::RandomProvider& randomProvider,
    const search::Assignment& assignment) const {
  return std::make_unique<search::Annealer>(
      randomProvider, std::move(_annealingScheduleFactory->create()),
      assignment);
}

void SolverThread::solve() {
  // Create the propagation solver
  propagation::Solver solver;
  solver.open();

  invariantgraph::SolverMapping mapping = _invariantGraph->construct(solver);

  solver.close();

  // retrieve the variables that are to be outputted
  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(_outputVarNodeIds.size());
  for (const auto oId : _outputVarNodeIds) {
    outputVarIds.emplace_back(mapping.solverId(oId));
  }

  search::Assignment assignment(
      solver, mapping.globalNeighborhood(), mapping.totalViolationId(), mapping.objectiveId(),
      mapping.objectiveDirection(), mapping.objectiveOptimalValue());

  // TODO: This can possibly be extracted, or restricted to one thread
  // TODO: this case may not be handled properly
  if (mapping.globalNeighborhood()->coveredVars().empty()) {
    _threadController->trySolution(
        _threadId, search::SavedAssignment(assignment, outputVarIds), nullptr);
    return;
  }

  search::RandomProvider randomProvider(_seed);
  search::SearchProcedure search(
      randomProvider, assignment, mapping.globalNeighborhood(),
      _searchType, _threadController, outputVarIds, _onMove, _threadId);

  search::SearchController searchController(
      mapping.objectiveDirection() == ObjectiveDirection::NONE, _timelimit,
      _shouldStop, _threadController);

  search.run(searchController,
             std::move(createMetaHeuristic(randomProvider, assignment)));
  _threadController->threadIsDone();
}

}  // namespace atlantis