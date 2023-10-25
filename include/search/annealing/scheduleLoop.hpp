#pragma once

#include <memory>
#include <optional>

#include "annealingSchedule.hpp"

namespace atlantis::search {

/**
 * Executes an inner schedule until a maximum number of consecutive rounds were
 * don't improve the assignment compared to the previous round.
 */
class ScheduleLoop : public AnnealingSchedule {
 private:
  std::unique_ptr<AnnealingSchedule> _schedule;
  UInt _maximumConsecutiveFutileRounds;

  UInt _consecutiveFutileIterations{0};
  std::optional<RoundStatistics> _lastRoundStatistics;

 public:
  explicit ScheduleLoop(std::unique_ptr<AnnealingSchedule> schedule,
                        UInt maximumConsecutiveFutileRounds)
      : _schedule(std::move(schedule)),
        _maximumConsecutiveFutileRounds(maximumConsecutiveFutileRounds) {}

  ~ScheduleLoop() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  bool frozen() override;
};

}  // namespace search
