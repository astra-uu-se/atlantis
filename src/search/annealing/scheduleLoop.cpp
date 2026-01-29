#include "atlantis/search/annealing/scheduleLoop.hpp"

namespace atlantis::search {

void ScheduleLoop::start(double initialTemperature) {
  _schedule->start(initialTemperature);
  _consecutiveFutileIterations = 0;
}

void ScheduleLoop::nextRound(
    const std::shared_ptr<RoundStatistics>& statistics) {
  assert(!frozen());

  const auto temp = temperature();
  _schedule->nextRound(statistics);

  if (_schedule->frozen()) {
    if (!_lastRoundStatistics) {
      _consecutiveFutileIterations =
          statistics->bestCostOfThisRound < statistics->bestCostOfPreviousRound
              ? 0
              : 1;
    } else if (statistics->bestCostOfThisRound <
               _lastRoundStatistics.value()->bestCostOfThisRound) {
      _consecutiveFutileIterations = 0;
    } else {
      ++_consecutiveFutileIterations;
    }

    _lastRoundStatistics.emplace(statistics);
    _schedule->start(temp);
  }
}

double ScheduleLoop::temperature() { return _schedule->temperature(); }

bool ScheduleLoop::frozen() {
  return _consecutiveFutileIterations >= _maximumConsecutiveFutileRounds;
}

}  // namespace atlantis::search
