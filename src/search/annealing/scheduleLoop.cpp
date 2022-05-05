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
    if (statistics.roundImprovedOnPrevious()) {
      _consecutiveFutileIterations = 0;
    } else {
      _consecutiveFutileIterations++;
    }

    _schedule->start(temp);
  }
}

double search::ScheduleLoop::temperature() { return _schedule->temperature(); }

UInt search::ScheduleLoop::numberOfMonteCarloSimulations() {
  return _schedule->numberOfMonteCarloSimulations();
}

bool search::ScheduleLoop::frozen() {
  return _consecutiveFutileIterations >= _maximumConsecutiveFutileRounds;
}
