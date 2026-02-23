#pragma once
#include <array>
#include <vector>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "pullResults.hpp"

namespace atlantis::search {

enum class BanditAlgorithm: unsigned char { ETC, LinUCB };
constexpr std::array<std::string_view, 2> banditAlgorithmNames = {"explore-then-commit", "linear UCB"};

class ArmSelector {
  const BanditAlgorithm _banditAlgorithm;
  mutable std::mutex _lock;
  const size_t _numArms;
  std::vector<size_t> _armScore;
  std::vector<size_t> _timesChosen;
  std::shared_ptr<AnnealingScheduleFactory>
      _annealingScheduleFactory;

  // Values specifically for the ETC algorithm
  const size_t _ETC_limit = 4;
  Int _ETC_bestArm = -1;

public:

  explicit ArmSelector(
     const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory,
     BanditAlgorithm banditAlgorithm);
  explicit ArmSelector(
     const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory);

  void recordArmStats(size_t arm,
                      const std::unique_ptr<PullResults> &stats);

  std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random);

  // Different bandit algorithm implementations.
  // These are called by chooseArm() based on the selected algortihm.
  size_t chooseArmETC();
};

}  // namespace atlantis::search
