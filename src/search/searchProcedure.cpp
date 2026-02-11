#include "atlantis/search/searchProcedure.hpp"

#include <chrono>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/metaheuristic.hpp"
#include "atlantis/search/searchController.hpp"

namespace atlantis::search {

std::shared_ptr<SearchStatistics> makeStats(
    const std::vector<std::shared_ptr<Statistic>>& inputStats) {
  auto stats = std::make_shared<SearchStatistics>();
  for (const auto& statistic : inputStats) {
    stats->insert(statistic);
  }
  return stats;
}

SavedAssignment SearchProcedure::saveAssignment() const {
  return SavedAssignment(_assignment, _outputVarIds);
}

void SearchProcedure::tightenSearch() {
  if (_searchType == SearchType::BESTCOST) {
    // TODO: need to keep track of best objective found to reduce communication.
    assert(false);
  }

}

bool SearchProcedure::onAccepted(
    const std::shared_ptr<CounterStatistic>& improvingSolutions) {
  // If a worsening move was accepted, there's no need to communicate
  if (_localBestAssignment.has_value() &&
      _localBestAssignment->cost() <= _assignment.getCost()) {
    return false;
  }

  // The found solution is better than any this thread has seen.
  _localBestAssignment = saveAssignment();

  // Is this the best global solution
  const bool isGlobalBest = _threadController->trySolution(
      _threadId, _localBestAssignment.value(), improvingSolutions);
  if (!isGlobalBest) {
    _localBestAssignment = _threadController->solution();
  }

  if (!_hasSolution && _localBestAssignment->cost().violation() == 0) {
    _hasSolution = true;
  }

  // If we are doing beam search, then update the current assignment:
  if (_searchType == SearchType::BEAMSEARCH) {
    _assignment.setAssignment(_localBestAssignment.value());
  }
  return true;
}

Int SearchProcedure::run(SearchController& searchController,
                         std::unique_ptr<MetaHeuristic>&& metaHeuristic) {
  const auto improvingSolutions =
      std::make_shared<CounterStatistic>("improvingSolutions");
  const auto communications =
      std::make_shared<CounterStatistic>("communications");
  // This counts the number of globally best solutions found by this thread.
  const auto stats = makeStats({improvingSolutions, communications});
  _threadController->setThreadStats(_threadId, stats);
  stats->setRoundStatistics(metaHeuristic->currentRoundStatistics());

  do {
    _assignment.initialize(_random);

    // TODO: handle this case: this should call some separate version
    if (_assignment.satisfiesConstraints()) {
      if (onAccepted(improvingSolutions)) communications->increment();
    }

    metaHeuristic->start();

    while (searchController.shouldRun(_assignment) &&
           !metaHeuristic->isFinished()) {
      const auto cost = _assignment.performProbe(_random);
      if (metaHeuristic->acceptMove(cost)) {
        _assignment.commitLastProbe();
        _onMove(*_threadController);
        if (!_hasSolution || _assignment.satisfiesConstraints()) {
          if (onAccepted(improvingSolutions)) communications->increment();
        }
      }
    }
  } while (searchController.shouldRun(_assignment));

  return 1;
}

}  // namespace atlantis::search
