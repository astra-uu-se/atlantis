#pragma once
#include <memory>

#include "armStats.hpp"
#include "pullResults.hpp"
#include "rewardController.hpp"

namespace atlantis::search {

class NumImprovementsRewardController : public RewardController {
  std::vector<std::shared_ptr<ArmStats>> _armStats;

public:

  explicit NumImprovementsRewardController(const std::vector<std::shared_ptr<ArmStats>>& armStats) :
    _armStats(armStats) {}

  [[nodiscard]] double getReward(const size_t arm, const PullResults& results) override {
    _armStats[arm]->addResult(results);
    const double points = results.improvingSolutions;

    printf("Arm %ld got cost %s and made %.f improvements.\n",
      arm, results._pullBestCost.value().toString().c_str(), points);

    return points;
  }
};

}  // namespace atlantis::search
