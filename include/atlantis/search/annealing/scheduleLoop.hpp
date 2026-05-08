#pragma once

#include <memory>
#include <optional>

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::search {

/**
 * Executes an inner schedule until a maximum number of consecutive rounds were
 * don't improve the assignment compared to the previous round.
 */
class ScheduleLoop : public AnnealingSchedule {
  std::unique_ptr<AnnealingSchedule> _schedule;
  UInt _maximumConsecutiveFutileRounds;

  UInt _consecutiveFutileIterations{0};
  std::optional<std::shared_ptr<RoundStatistics>> _lastRoundStatistics;

 public:
  explicit ScheduleLoop(std::unique_ptr<AnnealingSchedule>&& schedule,
                        UInt maximumConsecutiveFutileRounds)
      : _schedule(std::move(schedule)),
        _maximumConsecutiveFutileRounds(maximumConsecutiveFutileRounds) {}

  void start(double initialTemperature) override;
  void nextRound(const std::shared_ptr<RoundStatistics>& statistics) override;
  double temperature() override;
  bool frozen() override;
  [[nodiscard]] AnnealingSchedule& inner() { return *_schedule; }
};

}  // namespace atlantis::search
