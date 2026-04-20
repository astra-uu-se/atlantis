#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class ThompsonSampling : public ArmSelector {
  std::vector<double> _alpha;
  std::vector<double> _beta;

 public:
  explicit ThompsonSampling(const std::shared_ptr<AnnealingScheduleFactory>&
                                annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(
      RandomProvider& random) override;

  void printStats() const override;
};

}  // namespace atlantis::search
