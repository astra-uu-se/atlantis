#include "search/annealing/scheduleSequence.hpp"

void search::ScheduleSequence::start(double initialTemperature) {
  assert(initialTemperature != 0.0);

  _currentSchedule = 0;
  currentSchedule().start(initialTemperature);
}

void search::ScheduleSequence::nextRound(
    const search::RoundStatistics& statistics) {
  assert(!frozen());
  assert(!currentSchedule().frozen());

  auto temp = temperature();
  currentSchedule().nextRound(statistics);

  if (currentSchedule().frozen()) {
    _currentSchedule++;

    if (_currentSchedule < _schedules.size()) {
      currentSchedule().start(temp);
    }
  }
}

double search::ScheduleSequence::temperature() {
  return currentSchedule().temperature();
}

UInt search::ScheduleSequence::numberOfMonteCarloSimulations() {
  return currentSchedule().numberOfMonteCarloSimulations();
}

bool search::ScheduleSequence::frozen() {
  return _currentSchedule >= _schedules.size();
}

search::AnnealingSchedule& search::ScheduleSequence::currentSchedule() {
  return *_schedules[_currentSchedule];
}