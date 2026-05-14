#include "atlantis/search/searchProcedure.hpp"

#include <chrono>

#include "atlantis/search/bandits/pullResults.hpp"
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
    const std::shared_ptr<CounterStatistic>& improvingSolutions,
    std::unique_ptr<MetaHeuristic>&& metaHeuristic) {
  _pullResults.submitCost(_assignment.getCost());

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
      case SearchType::BESTCOST: {
        metaHeuristic->setCost(_localBestAssignment.value().cost());
      }
      case SearchType::BEAMSEARCH: {
        _assignment.setAssignment(_localBestAssignment.value());
      }
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

  do {
    std::unique_ptr<MetaHeuristic>&& metaHeuristic = std::make_unique<Annealer>(
      _random, _threadController->chooseArm(_threadId, _random), _assignment);

    auto roundStats = metaHeuristic->currentRoundStatistics();
    _assignment.initialize(_random);
    _pullResults.reset(_assignment.getCost(), roundStats);

    // TODO: handle this case: this should call some separate version
    if (_assignment.satisfiesConstraints()) {
      if (onAccepted(improvingSolutions, std::move(metaHeuristic)))
        communications->increment();
    }

    if (_searchType == SearchType::BEAMSEARCH && _localBestAssignment.has_value()) {
      _assignment.setAssignment(_localBestAssignment.value());
    }

    metaHeuristic->start();

    while (searchController.shouldRun(_assignment) &&
           !metaHeuristic->isFinished()) {

      const auto cost = _assignment.performProbe(_random);

      if (metaHeuristic->acceptMove(cost)) {
        _assignment.commitLastProbe();

        if (!_hasSolution || _assignment.satisfiesConstraints()) {
          if (onAccepted(improvingSolutions, std::move(metaHeuristic)))
            communications->increment();
        }
      }
    }

    restarts->increment();
    probes->setValue(probes->value() + roundStats.value()->attemptedMoves);
    moves->setValue(moves->value() + roundStats.value()->acceptedMoves);
    improvingMoves->setValue(improvingMoves->value() + roundStats.value()->improvingMoves);
    rounds->setValue(rounds->value() + roundStats.value()->rounds);

    _threadController->recordArm(_threadId, _pullResults);

  } while (searchController.shouldRun(_assignment));

  // if (_localBestAssignment.has_value())
  //   printf(
  //       "Thread %ld: SearchController stopped search at cost %s with %ld "
  //       "probes and %ld moves (%ld improving, %ld comms, %ld actual improvements, %ld rounds, %ld pulls). \n\t Using %s search. \n",
  //       _threadId, _localBestAssignment.value().cost().toString().c_str(),
  //       probes->value(), moves->value(),
  //       improvingMoves->value(), communications->value(), improvingSolutions->value(),
  //       rounds->value(), restarts->value(), searchTypeNames[static_cast<size_t>(_searchType)].data());

  return 1;
}

}  // namespace atlantis::search
