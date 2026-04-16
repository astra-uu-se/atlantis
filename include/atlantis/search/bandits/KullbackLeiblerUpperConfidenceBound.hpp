#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class KullbackLeiblerUpperConfidenceBound: public ArmSelector {
  size_t _totalPulls = 0;
  size_t _totalRecordedPulls = 0;
  std::vector<double> _meanPoints;

public:

  explicit KullbackLeiblerUpperConfidenceBound(
    const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  [[nodiscard]] std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) override;

  void printStats() const override;
};

} // namespace atlantis::search
