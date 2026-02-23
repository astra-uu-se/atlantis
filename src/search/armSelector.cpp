#include "atlantis/search/armSelector.hpp"

namespace atlantis::search {

ArmSelector::ArmSelector(
   const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory, BanditAlgorithm banditAlgorithm) :
  _banditAlgorithm(banditAlgorithm),
  _numArms(annealingScheduleFactory->armCount()),
  _annealingScheduleFactory(annealingScheduleFactory) {
  _armScore = std::vector<size_t>(_numArms, 0);
  _timesChosen = std::vector<size_t>(_numArms, 0);
}

ArmSelector::ArmSelector(
   const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory) :
ArmSelector(annealingScheduleFactory, BanditAlgorithm::ETC) {}

// TODO: Create some sort of per-Arm previous stats value somewhere.
void ArmSelector::recordArmStats(const size_t arm,
                    const std::unique_ptr<PullResults> &stats) {
  _lock.lock();
  // This should require locks only for the actual update.
  _armScore[arm] += stats->improvingSolutions;
  _lock.unlock();
}

std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ArmSelector::chooseArm(RandomProvider& random) {
  size_t arm;
  switch (_banditAlgorithm) {
    case BanditAlgorithm::ETC:
      arm = chooseArmETC();
      break;
    default:
      printf("This selection algorithm has not been implemented. Defaulting to arm 0.\n");
      arm = 0;
  }

  // printf("Choosing arm %ld. Arm stats [%ld, %ld], times chosen [%ld, %ld]\n", arm, _armScore[0], _armScore[1], _timesChosen[0], _timesChosen[1]);
  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

size_t ArmSelector::chooseArmETC() {
  Int arm = -1;

  _lock.lock();

  if (_ETC_bestArm < 0) {
    size_t bestArm = 0;
    for (size_t i = 0; i < _numArms; ++i) {
      if (_timesChosen[i] < _ETC_limit) {
        arm = i;
        break;
      }
      if (_armScore[i] > _armScore[bestArm]) {
        bestArm = i;
      }
    }

    if (arm < 0) {
      _ETC_bestArm = bestArm;
      arm = _ETC_bestArm;
    }
  }
  else { arm = _ETC_bestArm;}

  _timesChosen[arm]++;
  _lock.unlock();

  return arm;
}

}  // namespace atlantis::search
