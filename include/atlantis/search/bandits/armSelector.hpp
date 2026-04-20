#pragma once

#include <array>
#include <iostream>
#include <memory>
#include <vector>

#include "armStats.hpp"
#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "pullResults.hpp"
#include "rewardController.hpp"

namespace atlantis::search {

enum class BanditAlgorithm : unsigned char { ETC, Thompson, UCB, KLUCB };
constexpr std::array<std::string_view, 4> banditAlgorithmNames = {
    "explore-then-commit", "Thompson sampling", "UCB", "KL-UCB"};

class ArmSelector {
 protected:
  mutable std::mutex _lock;
  size_t _numArms = 0;
  size_t _totalPulls = 0;
  size_t _totalRecordedPulls = 0;
  std::vector<std::shared_ptr<ArmStats>> _armStats;
  std::shared_ptr<AnnealingScheduleFactory> _annealingScheduleFactory;
  std::unique_ptr<RewardController> _rewardController;

 public:
  explicit ArmSelector(
      const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
      : _numArms(annealingScheduleFactory->armCount()),
        _annealingScheduleFactory(annealingScheduleFactory) {
    _armStats = std::vector<std::shared_ptr<ArmStats>>();
    for (size_t arm = 0; arm < _numArms; arm++) {
      _armStats.push_back(std::make_shared<ArmStats>());
    }
  }

  virtual ~ArmSelector() = default;

  virtual void recordArmStats(size_t arm, const PullResults& stats) = 0;

  virtual std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(
      RandomProvider& random) = 0;

  virtual void printStats() const {
    printf("\n");
    for (size_t arm = 0; arm < _numArms; arm++) {
      const auto a = _armStats[arm]->toString();
      std::cout << "Arm " << arm << ": " << a << std::endl;
    }
  }
};

}  // namespace atlantis::search
