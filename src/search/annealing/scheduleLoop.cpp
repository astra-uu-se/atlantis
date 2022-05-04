#include "search/annealing/scheduleLoop.hpp"

void search::ScheduleLoop::start(double initialTemperature) {
  _schedule->start(initialTemperature);
  _executedIterations = 0;
}

void search::ScheduleLoop::nextRound(
    const search::RoundStatistics& statistics) {
  assert(!frozen());

  auto temp = temperature();
  _schedule->nextRound(statistics);

  if (_schedule->frozen()) {
    _executedIterations++;
    _schedule->start(temp);
  }
}

double search::ScheduleLoop::temperature() { return _schedule->temperature(); }

UInt search::ScheduleLoop::numberOfMonteCarloSimulations() {
  return _schedule->numberOfMonteCarloSimulations();
}

bool search::ScheduleLoop::frozen() {
  return _executedIterations >= _numberOfIterations;
}
