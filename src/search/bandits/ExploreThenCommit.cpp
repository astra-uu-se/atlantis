#include "atlantis/search/bandits/ExploreThenCommit.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"

namespace atlantis::search {

ExploreThenCommit::ExploreThenCommit(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _means = std::vector<double>(_numArms, 0);
}


void ExploreThenCommit::recordArmStats(const size_t arm,
                    const PullResults& stats) {
  printf("Arm %ld got result %ld and cost %s.\n", arm, stats.improvingSolutions, stats._pullBestCost.value().toString().c_str());
  std::lock_guard lock(_lock);
  double reward = _armStats[arm].addResult(stats);

  double newMean = (_means[arm] * (size(_armStats[arm].rewards) - 1) + reward) / size(_armStats[arm].rewards) ;
  _means[arm] = newMean;
}


std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ExploreThenCommit::chooseArm(RandomProvider&) {
  Int arm = -1;

  _lock.lock();

  if (_ETC_bestArm < 0) {
    size_t bestArm = 0;
    for (size_t i = 0; i < _numArms; ++i) {
      if (_armStats[i].timesChosen < _ETC_limit) {
        arm = i;
        break;
      }
      if (_means[i] > _means[bestArm]) {
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

  printf("  Choosing arm %ld. Arm means [%.4f, %.4f], times chosen [%ld, %ld].\n", arm, _means[0], _means[1], _armStats[0].timesChosen, _armStats[1].timesChosen);
  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

}  // namespace atlantis::search
