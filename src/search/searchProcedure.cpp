#include "atlantis/search/searchProcedure.hpp"

#include <chrono>

#include "../../include/atlantis/search/bandits/pullResults.hpp"
#include "atlantis/search/annealing/annealer.hpp"
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

bool SearchProcedure::onAccepted(
    const std::shared_ptr<CounterStatistic>& improvingSolutions, std::unique_ptr<MetaHeuristic>&& metaHeuristic) {
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
    // TODO: this can be optimized; only the cost is needed for other than Beamsearch.
    _localBestAssignment = _threadController->solution();

    switch (_searchType) {
      case SearchType::BESTCOST: { metaHeuristic->setCost(_localBestAssignment.value().cost()); }
      case SearchType::BEAMSEARCH: { _assignment.setAssignment(_localBestAssignment.value()); }
      default:;
    }
  }

  if (!_hasSolution && _localBestAssignment->cost().violation() == 0) {
    _hasSolution = true;
  }

  return true;
}

Int SearchProcedure::run(SearchController& searchController) {
  // This counts the number of globally best solutions found by this thread.
  const auto improvingSolutions = std::make_shared<CounterStatistic>("improvingSolutions");
  const auto communications = std::make_shared<CounterStatistic>("communications");
  const auto probes = std::make_shared<CounterStatistic>("probes");
  const auto moves = std::make_shared<CounterStatistic>("moves");
  const auto improvingMoves = std::make_shared<CounterStatistic>("improvingMoves");
  const auto rounds = std::make_shared<CounterStatistic>("rounds");
  const auto restarts = std::make_shared<CounterStatistic>("restarts");
  const auto stats = makeStats({improvingSolutions, communications, probes, moves, improvingMoves, rounds, restarts});
  _threadController->setThreadStats(_threadId, stats);

  auto pullResults = std::make_unique<PullResults>(0);

#ifdef MORE_STATS
  std::chrono::system_clock::time_point startProbe;
  std::chrono::system_clock::time_point startScheduleFactory;
  std::chrono::system_clock::time_point startCommit;
  double probeTime = 0;
  double fullProbeTime = 0;
  double commitTime = 0;
  double scheduleTime = 0;
#endif

  do {
#ifdef MORE_STATS
    startScheduleFactory = std::chrono::high_resolution_clock::now();
#endif
    std::unique_ptr<MetaHeuristic>&& metaHeuristic = std::make_unique<Annealer>(
      _random, _threadController->chooseArm(_threadId, pullResults, _random), _assignment);
#ifdef MORE_STATS
    scheduleTime +=
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - startScheduleFactory)
            .count();
#endif

    auto roundStats = metaHeuristic->currentRoundStatistics();
    _assignment.initialize(_random);

    // TODO: handle this case: this should call some separate version
    if (_assignment.satisfiesConstraints()) {
      if (onAccepted(improvingSolutions, std::move(metaHeuristic))) communications->increment();
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
          if (onAccepted(improvingSolutions, std::move(metaHeuristic))) communications->increment();
        }
      }

#ifdef MORE_STATS
      fullProbeTime +=
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now() - startProbe)
              .count();
#endif
    }

    // TODO: Inject MAB communication here

    restarts->increment();
    probes->setValue(probes->value() + roundStats.value()->attemptedMoves);
    moves->setValue(moves->value() + roundStats.value()->acceptedMoves);
    improvingMoves->setValue(improvingMoves->value() + roundStats.value()->improvingMoves);
    rounds->setValue(rounds->value() + roundStats.value()->rounds);

    pullResults = PullResults::newResults(pullResults, improvingSolutions->value());
  } while (searchController.shouldRun(_assignment));

#ifdef MORE_STATS
  double avgProbeTime = probeTime / probes->value();
  double avgFullProbeTime = fullProbeTime / probes->value();
  double avgCommitTime = commitTime / probes->value();
  double avgScheduleTime = scheduleTime / restarts->value();
  if (_localBestAssignment.has_value())
    printf(
        "Thread %ld: SearchController stopped search at cost %s with %ld "
        "probes and %ld moves (%ld improving, %ld comms, %ld actual improvements, %ld rounds, %ld restarts). \n\t Average "
        "full probe time %.4f, probe time %.4f ms, commit time %.4f ms, schedule generating time %.4f ms. Using %s search. \n",
        _threadId, _localBestAssignment.value().cost().toString().c_str(),
        probes->value(), moves->value(),
        improvingMoves->value(), communications->value(), improvingSolutions->value(),
        rounds->value(), restarts->value(), avgFullProbeTime, avgProbeTime,
        avgCommitTime, avgScheduleTime, searchTypeNames[static_cast<size_t>(_searchType)].data());
#endif

  return 1;
}

}  // namespace atlantis::search
