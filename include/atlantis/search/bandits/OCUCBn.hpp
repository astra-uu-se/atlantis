#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class OCUCBn : public ArmSelector {
  std::vector<double> _meanPoints;

  // For OCUCB-n
  double _eta = 1.1; // Must be > 1
  double _rho = 1/2; // Must be in [1/2, 1]

  double b(size_t i, size_t t) const;
  double gamma(size_t i, size_t t) const;

public:
  explicit OCUCBn(
      const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory,
      const std::function<void(std::shared_ptr<ArmStats>, size_t)>& onArmRecording);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  [[nodiscard]] std::tuple<std::unique_ptr<AnnealingSchedule>, size_t>
  chooseArm(RandomProvider& random) override;

  void printStats() const override;
};

}  // namespace atlantis::search
