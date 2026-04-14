#include "atlantis/search/bandits/ExploreThenCommit.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"

namespace atlantis::search {

ExploreThenCommit::ExploreThenCommit(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _means = std::vector<std::shared_ptr<Reward>>(_numArms);
  for (size_t i = 0; i < _numArms; ++i) {
    _means[i] = _rewardFactory->initAverageReward();
  }
}


void ExploreThenCommit::recordArmStats(const size_t arm,
                    const PullResults& stats) {
  printf("Arm %ld got result %ld and cost %s.\n", arm, stats.improvingSolutions, stats._pullBestCost.value().toString().c_str());
  std::lock_guard lock(_lock);
  const std::shared_ptr<Reward> reward = _armStats[arm].addResult(stats);
  _means[arm]->updateAverage(reward, size(_armStats[arm].rewards));
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

  // printf("  Choosing arm %ld. Arm means [%s, %s], times chosen [%ld, %ld].\n",
  //   arm, _means[0]->toString().c_str(), _means[1]->toString().c_str(),
  //   _armStats[0].timesChosen, _armStats[1].timesChosen);
  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

}  // namespace atlantis::search
