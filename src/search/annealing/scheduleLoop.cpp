#include "search/annealing/scheduleLoop.hpp"

void search::ScheduleLoop::start(double initialTemperature) {
  _schedule->start(initialTemperature);
  _consecutiveFutileIterations = 0;
}

void search::ScheduleLoop::nextRound(
    const search::RoundStatistics& statistics) {
  assert(!frozen());

  auto temp = temperature();
  _schedule->nextRound(statistics);

  if (_schedule->frozen()) {
    if (_lastRoundStatistics && _lastRoundStatistics->bestCostOfThisRound == statistics.bestCostOfThisRound) {
      _consecutiveFutileIterations++;
    } else {
      _consecutiveFutileIterations = 0;
    }

    _lastRoundStatistics.emplace(statistics);
    _schedule->start(temp);
  }
}

double search::ScheduleLoop::temperature() { return _schedule->temperature(); }

bool search::ScheduleLoop::frozen() {
  return _consecutiveFutileIterations >= _maximumConsecutiveFutileRounds;
}
