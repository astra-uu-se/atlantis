#pragma once
#include <vector>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::search {

class ArmSelector {
  const size_t _numArms;
  std::vector<size_t> _armScore;
  std::shared_ptr<AnnealingScheduleFactory>
      _annealingScheduleFactory;

public:

  explicit ArmSelector(
     const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory) :
    _numArms(annealingScheduleFactory->armCount()),
    _annealingScheduleFactory(annealingScheduleFactory) {
    _armScore.reserve(_numArms);
  }

  std::unique_ptr<AnnealingSchedule> chooseArm(RandomProvider& random) const {
    // TEMP SOLUTION for testing
    const size_t arm = random.intInRange(0, _numArms - 1);
    return _annealingScheduleFactory->create(arm);
  }
};

}  // namespace atlantis::search
