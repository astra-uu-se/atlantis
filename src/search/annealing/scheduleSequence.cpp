#include "atlantis/search/annealing/scheduleSequence.hpp"

#include "atlantis/search/annealing/types.hpp"

namespace atlantis::search {

void ScheduleSequence::start(double initialTemperature) {
  assert(initialTemperature != 0.0);

  _currentSchedule = 0;
  currentSchedule().start(initialTemperature);
}

void ScheduleSequence::nextRound(
    const std::shared_ptr<RoundStatistics>& statistics) {
  assert(!frozen());

  const auto temp = temperature();
  currentSchedule().nextRound(statistics);

  if (currentSchedule().frozen()) {
    _currentSchedule++;

    if (_currentSchedule < _schedules.size()) {
      currentSchedule().start(temp);
    }
  }
}

double ScheduleSequence::temperature() const {
  return currentScheduleConst().temperature();
}

bool ScheduleSequence::frozen() const {
  return _currentSchedule >= _schedules.size();
}

std::unique_ptr<AnnealingSchedule> ScheduleSequence::clone() const {
  std::vector<std::unique_ptr<AnnealingSchedule>> schedules;
  schedules.reserve(_schedules.size());
  for (const auto& schedule : _schedules) {
    schedules.emplace_back(schedule->clone());
  }
  return std::make_unique<ScheduleSequence>(std::move(schedules));
}

const AnnealingSchedule& ScheduleSequence::currentScheduleConst() const {
  return *_schedules[_currentSchedule];
}

AnnealingSchedule& ScheduleSequence::currentSchedule() {
  return *_schedules[_currentSchedule];
}

}  // namespace atlantis::search
