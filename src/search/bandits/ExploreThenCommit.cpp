#include "atlantis/search/bandits/ExploreThenCommit.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"

namespace atlantis::search {

ExploreThenCommit::ExploreThenCommit(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {}


// TODO: Create some sort of per-Arm previous stats value somewhere.
void ExploreThenCommit::recordArmStats(const size_t arm,
                    const std::unique_ptr<PullResults> &stats) {
  _lock.lock();
  // This should require locks only for the actual update.
  // TODO: make armStats.addResult take the entire stats object.
  _armStats[arm].addResult(stats->improvingSolutions);
  _lock.unlock();
}


std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ExploreThenCommit::chooseArm(RandomProvider& random) {
  Int arm = -1;

  _lock.lock();

  if (_ETC_bestArm < 0) {
    size_t bestArm = 0;
    for (size_t i = 0; i < _numArms; ++i) {
      if (_armStats[i].timesChosen < _ETC_limit) {
        arm = i;
        break;
      }
      if (_armStats[i].mean > _armStats[bestArm].mean) {
        bestArm = i;
      }
    }

    if (arm < 0) {
      _ETC_bestArm = bestArm;
      arm = _ETC_bestArm;
    }
  }
  else { arm = _ETC_bestArm;}

  _armStats[arm].timesChosen++;
  _lock.unlock();

  printf("Choosing arm %ld. Arm stats [%.4f, %.4f], times chosen [%ld, %ld].\n", arm, _armStats[0].mean, _armStats[1].mean, _armStats[0].timesChosen, _armStats[1].timesChosen);
  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

// size_t ArmSelector::chooseArm() {
//   Int arm = -1;
//
//   _lock.lock();
//
//   if (_ETC_bestArm < 0) {
//     size_t bestArm = 0;
//     for (size_t i = 0; i < _numArms; ++i) {
//       if (_armStats[i].timesChosen < _ETC_limit) {
//         arm = i;
//         break;
//       }
//       if (_armStats[i].mean > _armStats[bestArm].mean) {
//         bestArm = i;
//       }
//     }
//
//     if (arm < 0) {
//       _ETC_bestArm = bestArm;
//       arm = _ETC_bestArm;
//     }
//   }
//   else { arm = _ETC_bestArm;}
//
//   _armStats[arm].timesChosen++;
//   _lock.unlock();
//
//   return arm;
// }

}  // namespace atlantis::search
