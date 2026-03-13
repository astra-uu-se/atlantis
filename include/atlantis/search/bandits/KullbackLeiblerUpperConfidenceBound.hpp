#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class KullbackLeiblerUpperConfidenceBound: public ArmSelector {
  size_t _totalPulls = 0;
  size_t _totalRecordedPulls = 0;
  std::vector<double> _meanRewards;
  Int _bestCost = INT_MAX;

  // Extra stats stuff
  std::vector<double> _meanProbes;
  std::vector<double> _meanMoves;
  std::vector<double> _meanImprovingMoves;
  std::vector<double> _meanRounds;



public:

  explicit KullbackLeiblerUpperConfidenceBound(
    const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  [[nodiscard]] std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) override;

  void printStats() const override;
};

} // namespace atlantis::search
