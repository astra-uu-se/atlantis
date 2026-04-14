#pragma once
#include <memory>
#include "numImprovementsReward.hpp"
#include "pullResults.hpp"

namespace atlantis::search {

enum class RewardType: unsigned char { NumImprovements };

class RewardFactory {
  const RewardType _rewardType;

public:
  explicit RewardFactory(const RewardType rewardType) : _rewardType{rewardType} {}

  std::shared_ptr<Reward> makeReward(const PullResults& result) const {
    switch (_rewardType) {
      case RewardType::NumImprovements:
        return std::make_shared<NumImprovementsReward>(result);
    }
    throw std::invalid_argument("Function makeReward not implemented for this reward type!");
  }

  std::shared_ptr<Reward> initAverageReward() const {
    switch (_rewardType) {
      case RewardType::NumImprovements:
        return NumImprovementsReward::average(std::nullopt);
    }
    throw std::invalid_argument("Function makeAverageReward not implemented for this reward type!");
  }

  std::shared_ptr<Reward> makeAverageReward(const std::vector<std::shared_ptr<Reward>>& rewards) const {
    switch (_rewardType) {
      case RewardType::NumImprovements:
        return NumImprovementsReward::average(rewards);
    }
    throw std::invalid_argument("Function makeAverageReward not implemented for this reward type!");
  }
};

}  // namespace atlantis::search
