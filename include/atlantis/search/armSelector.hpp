#pragma once
#include <vector>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "pullResults.hpp"

#define ETC_LIMIT 4

namespace atlantis::search {

class ArmSelector {
  mutable std::mutex _lock;
  const size_t _numArms;
  std::vector<size_t> _armScore;
  std::vector<size_t> _timesChosen;
  std::shared_ptr<AnnealingScheduleFactory>
      _annealingScheduleFactory;

  Int _bestArm = -1;

public:

  explicit ArmSelector(
     const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory) :
    _numArms(annealingScheduleFactory->armCount()),
    _annealingScheduleFactory(annealingScheduleFactory) {
    _armScore = std::vector<size_t>(_numArms, 0);
    _timesChosen = std::vector<size_t>(_numArms, 0);
  }

  // TODO: Create some sort of per-Arm previous stats value somewhere.
  void recordArmStats(const size_t arm,
                      const std::unique_ptr<PullResults> &stats) {

    _lock.lock();
    // This should require locks only for the actual update.
      _armScore[arm] += stats->improvingSolutions;
    _lock.unlock();
  }

  std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) {
    // TEMP SOLUTION for testing
    // printf("Arm stats [%ld, %ld], times chosen [%ld, %ld]\n", _armScore[0], _armScore[1], _timesChosen[0], _timesChosen[1]);
    Int arm = -1;
    // arm = random.intInRange(0, _numArms - 1);

    _lock.lock();

    if (_bestArm < 0) {
      size_t bestArm = 0;
      for (size_t i = 0; i < _numArms; ++i) {
        if (_timesChosen[i] < ETC_LIMIT) {
          arm = i;
          break;
        }
        if (_armScore[i] > _armScore[bestArm]) {
          bestArm = i;
        }
      }

      if (arm < 0) {
        _bestArm = bestArm;
        arm = _bestArm;
      }
    }
    else { arm = _bestArm;}

    _timesChosen[arm]++;
    _lock.unlock();

    return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
  }
};

}  // namespace atlantis::search
