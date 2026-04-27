#include "atlantis/search/bandits/ExploreThenCommit.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"
#include "atlantis/search/bandits/costRewardController.hpp"
#include "atlantis/search/bandits/numImprovementsRewardController.hpp"

namespace atlantis::search {

ExploreThenCommit::ExploreThenCommit(
const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory,
const std::function<void(std::shared_ptr<ArmStats>, size_t)>& onArmRecording)
    : ArmSelector(annealingScheduleFactory, onArmRecording) {
  _rewardController = std::make_unique<NumImprovementsRewardController>(_armStats);
  _means = std::vector(_numArms, 0.0);
}


void ExploreThenCommit::recordArmStats(const size_t arm,
                    const PullResults& stats) {
  std::lock_guard lock(_lock);

  const double points = _rewardController->getReward(arm, stats);

  _means[arm] = (_means[arm] * (_armStats[arm]->timesRecorded - 1) + points) /
                _armStats[arm]->timesRecorded;

  _onArmRecording(_armStats[arm], arm);
}


std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ExploreThenCommit::chooseArm(RandomProvider&) {
  Int arm = -1;

  _lock.lock();

  if (_ETC_bestArm < 0) {
    size_t bestArm = 0;
    for (size_t i = 0; i < _numArms; ++i) {
      if (_armStats[i]->timesChosen < _ETC_limit) {
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

  _armStats[arm]->timesChosen++;
  _lock.unlock();

  // printf("  Choosing arm %ld. Arm means [%s, %s], times chosen [%ld, %ld].\n",
  //   arm, _means[0]->toString().c_str(), _means[1]->toString().c_str(),
  //   _armStats[0].timesChosen, _armStats[1].timesChosen);
  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

}  // namespace atlantis::search
