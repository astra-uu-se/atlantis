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
  // This counts the number of globally best solutions found by this thread.
  const auto improvingSolutions =
      std::make_shared<CounterStatistic>("improvingSolutions");
  const auto communications =
      std::make_shared<CounterStatistic>("communications");
  const auto stats = makeStats({improvingSolutions, communications});
  _threadController->setThreadStats(_threadId, stats);
  auto roundStats = metaHeuristic->currentRoundStatistics();
  stats->setRoundStatistics(roundStats);

#ifdef MORE_STATS
  std::chrono::system_clock::time_point startProbe;
  std::chrono::system_clock::time_point startCommit;
  double probeTime = 0;
  double fullProbeTime = 0;
  double commitTime = 0;
#endif

  do {
    _assignment.initialize(_random);

    // TODO: handle this case: this should call some separate version
    if (_assignment.satisfiesConstraints()) {
      if (onAccepted(improvingSolutions)) communications->increment();
    }

    metaHeuristic->start();

    while (searchController.shouldRun(_assignment) &&
           !metaHeuristic->isFinished()) {

#ifdef MORE_STATS
      startProbe = std::chrono::high_resolution_clock::now();
#endif

      const auto cost = _assignment.performProbe(_random);

#ifdef MORE_STATS
      probeTime += std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::high_resolution_clock::now() - startProbe)
                       .count();
#endif

      if (metaHeuristic->acceptMove(cost)) {
#ifdef MORE_STATS
        startCommit = std::chrono::high_resolution_clock::now();
#endif

        _assignment.commitLastProbe();

#ifdef MORE_STATS
        commitTime +=
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - startCommit)
                .count();
#endif

        if (!_hasSolution || _assignment.satisfiesConstraints()) {
          if (onAccepted(improvingSolutions)) communications->increment();
        }
      }

#ifdef MORE_STATS
      fullProbeTime +=
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now() - startProbe)
              .count();
#endif
    }
  } while (searchController.shouldRun(_assignment));

#ifdef MORE_STATS
  double avgProbeTime = probeTime / roundStats.value()->attemptedMoves;
  double avgFullProbeTime = fullProbeTime / roundStats.value()->attemptedMoves;
  double avgCommitTime = commitTime / roundStats.value()->attemptedMoves;
  if (_localBestAssignment.has_value())
    printf(
        "Thread %ld: SearchController stopped search at cost %s with %ld "
        "probes and %ld moves (%ld improving, %s comms, %ld restarts). \n\t Average "
        "full probe time %.4f, probe time %.4f ms, commit time %.4f ms. Using %s search.\n",
        _threadId, _localBestAssignment.value().cost().toString().c_str(),
        roundStats.value()->attemptedMoves, roundStats.value()->acceptedMoves,
        roundStats.value()->improvingMoves, communications->value().c_str(),
        roundStats.value()->rounds, avgFullProbeTime, avgProbeTime,
        avgCommitTime, searchTypeNames[static_cast<size_t>(_searchType)].data());
#endif

  return 1;
}

}  // namespace atlantis::search
