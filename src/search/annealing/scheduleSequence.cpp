#include "atlantis/search/annealing/scheduleSequence.hpp"

namespace atlantis::search {

void ScheduleSequence::start(double initialTemperature) {
  assert(initialTemperature != 0.0);

  _currentSchedule = 0;
  currentSchedule().start(initialTemperature);
}

void ScheduleSequence::nextRound(const RoundStatistics& statistics) {
  assert(!frozen());

  auto temp = temperature();
  currentSchedule().nextRound(statistics);

  if (currentSchedule().frozen()) {
    _currentSchedule++;

    if (_currentSchedule < _schedules.size()) {
      currentSchedule().start(temp);
    }
  }
}

double ScheduleSequence::temperature() {
  return currentSchedule().temperature();
}

bool ScheduleSequence::frozen() {
  return _currentSchedule >= _schedules.size();
}

AnnealingSchedule& ScheduleSequence::currentSchedule() {
  return *_schedules[_currentSchedule];
}

}  // namespace atlantis::search
