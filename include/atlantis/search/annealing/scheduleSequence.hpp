#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "atlantis/search/annealing/annealingSchedule.hpp"

namespace atlantis::search {

class ScheduleSequence : public AnnealingSchedule {
 public:
  using ScheduleList = std::vector<std::unique_ptr<AnnealingSchedule>>;

 private:
  ScheduleList _schedules;
  size_t _currentSchedule{0};
  AnnealingSchedule& currentSchedule();

 public:
  explicit ScheduleSequence(ScheduleList schedules)
      : _schedules(std::move(schedules)) {
    assert(!_schedules.empty());
  }

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  bool frozen() override;
  [[nodiscard]] size_t size() const { return _schedules.size(); }
  [[nodiscard]] AnnealingSchedule& at(size_t index) {
    return *(_schedules.at(index));
  }
};

}  // namespace atlantis::search
