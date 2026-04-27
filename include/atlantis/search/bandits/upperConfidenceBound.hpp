#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class UpperConfidenceBound : public ArmSelector {
  std::vector<double> _meanPoints;

 public:
  explicit UpperConfidenceBound(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory,
    const std::function<void(std::shared_ptr<ArmStats>, size_t)>& onArmRecording);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  [[nodiscard]] std::tuple<std::unique_ptr<AnnealingSchedule>, size_t>
  chooseArm(RandomProvider& random) override;

  void printStats() const override;
};

}  // namespace atlantis::search
