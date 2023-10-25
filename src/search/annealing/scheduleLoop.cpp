#include "search/annealing/scheduleLoop.hpp"

namespace atlantis::search {

void ScheduleLoop::start(double initialTemperature) {
  _schedule->start(initialTemperature);
  _consecutiveFutileIterations = 0;
}

void ScheduleLoop::nextRound(const RoundStatistics& statistics) {
  assert(!frozen());

  auto temp = temperature();
  _schedule->nextRound(statistics);

  if (_schedule->frozen()) {
    if (_lastRoundStatistics && _lastRoundStatistics->bestCostOfThisRound <=
                                    statistics.bestCostOfThisRound) {
      _consecutiveFutileIterations++;
    } else {
      _consecutiveFutileIterations = 1;
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