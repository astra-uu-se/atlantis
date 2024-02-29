#pragma once

#include <memory>
#include <vector>

#include "annealingSchedule.hpp"

namespace atlantis::search {

class ScheduleSequence : public AnnealingSchedule {
 public:
  using ScheduleList = std::vector<std::unique_ptr<AnnealingSchedule>>;

 private:
  ScheduleList _schedules;

  size_t _currentSchedule{0};

 public:
  explicit ScheduleSequence(ScheduleList schedules)
      : _schedules(std::move(schedules)) {
    assert(!_schedules.empty());
  }

  ~ScheduleSequence() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  bool frozen() override;

 private:
  AnnealingSchedule& currentSchedule();
};

}  // namespace atlantis::search
