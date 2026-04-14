#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class ThompsonSampling : public ArmSelector {
  size_t _totalPulls = 0;
  std::vector<double> _alpha;
  std::vector<double> _beta;

  std::vector<double> _meanProbes;
  std::vector<double> _meanMoves;
  std::vector<double> _meanImprovingMoves;
  std::vector<double> _meanRounds;
  std::vector<std::shared_ptr<Reward>> _meanRewards;

  size_t _totalRecordedPulls = 0;

public:

  explicit ThompsonSampling(
    const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) override;

  void printStats() const override;
};

} // namespace atlantis::search