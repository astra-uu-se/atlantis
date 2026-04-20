#pragma once
#include <memory>

#include "armStats.hpp"
#include "pullResults.hpp"
#include "rewardController.hpp"

namespace atlantis::search {

class IsImprovementsRewardController : public RewardController {
  std::vector<std::shared_ptr<ArmStats>> _armStats;
  std::optional<Cost> _bestCost;

public:

  explicit IsImprovementsRewardController(const std::vector<std::shared_ptr<ArmStats>>& armStats) :
    _armStats(armStats) {}

  [[nodiscard]] double getReward(const size_t arm, const PullResults& results) override {
    if (!results._pullBestCost.has_value()) {
      throw std::runtime_error("Results doesn't contain reward value!");
    }

    _armStats[arm]->addResult(results);
    const Cost result = results._pullBestCost.value();

    double points = 0;
    if (!_bestCost.has_value() || result < _bestCost.value()) {
      _bestCost = result;
      points = 1;
    }

    printf("Arm %ld got cost %s and best is %s, so it gets %.0f point.\n",
      arm, results._pullBestCost.value().toString().c_str(),
      _bestCost.value().toString().c_str(), points);

    return points;
  }
};

}  // namespace atlantis::search
