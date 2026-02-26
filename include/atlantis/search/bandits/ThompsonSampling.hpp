#pragma once

#include "armSelector.hpp"

namespace atlantis::search {

class ThompsonSampling : public ArmSelector {
  bool _initialRound = true;
  bool _hasInitializedValues = false;
  bool _initializationDone = false;
  size_t _initialRoundNextArm = 0;
  size_t _totalPulls = 0;

  double _globalMean = 0;
  std::vector<double> _alpha;
  std::vector<double> _beta;

public:

  explicit ThompsonSampling(
    const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) override;
};

} // namespace atlantis::search