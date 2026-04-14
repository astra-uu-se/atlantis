#pragma once

#include "armSelector.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

class ExploreThenCommit : public ArmSelector {
  // Values specifically for the ETC algorithm
  const size_t _ETC_limit = 4;
  Int _ETC_bestArm = -1;
  std::vector<std::shared_ptr<Reward>> _means;

public:

  explicit ExploreThenCommit(
    const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory);

  void recordArmStats(size_t arm, const PullResults& stats) override;

  std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) override;
};

} // namespace atlantis::search