#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class UpperConfidenceBound : public ArmSelector {
  std::vector<double> _meanPoints;
  std::vector<double> _cumulativePoints;

 public:
  explicit UpperConfidenceBound(const std::shared_ptr<AnnealingScheduleFactory>&
                                    annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  [[nodiscard]] std::tuple<std::unique_ptr<AnnealingSchedule>, size_t>
  chooseArm(RandomProvider& random) override;

  void printStats() const override;
};

}  // namespace atlantis::search
