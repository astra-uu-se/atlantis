#pragma once

#include <memory>

#include "annealingSchedule.hpp"

namespace search {

class ScheduleLoop : public AnnealingSchedule {
 private:
  std::unique_ptr<AnnealingSchedule> _schedule;
  UInt _numberOfIterations;

  UInt _executedIterations{0};

 public:
  explicit ScheduleLoop(std::unique_ptr<AnnealingSchedule> schedule,
                        UInt numberOfIterations)
      : _schedule(std::move(schedule)),
        _numberOfIterations(numberOfIterations) {}

  ~ScheduleLoop() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  UInt numberOfMonteCarloSimulations() override;
  bool frozen() override;
};

}  // namespace search
