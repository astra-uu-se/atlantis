#include "atlantis/search/bandits/upperConfidenceBound.hpp"

#include <cmath>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/bandits/costRewardController.hpp"
#include "atlantis/search/bandits/isImprovementsRewardController.hpp"

namespace atlantis::search {

UpperConfidenceBound::UpperConfidenceBound(
const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory,
const std::function<void(std::shared_ptr<ArmStats>, size_t)>& onArmRecording)
    : ArmSelector(annealingScheduleFactory, onArmRecording) {
  _rewardController = std::make_unique<CostRewardController>(_armStats);

  _meanPoints = std::vector(_numArms, 0.0);
}

static double calcNewMean(const double oldMean, const double newValue,
                          const double newN) {
  return (oldMean * (newN - 1) + newValue) / newN;
}

void UpperConfidenceBound::recordArmStats(const size_t arm,
                                          const PullResults& stats) {
  std::lock_guard lock(_lock);
  _totalRecordedPulls++;

  const double points = _rewardController->getReward(arm, stats);

  const size_t n = _armStats[arm]->timesRecorded;
  _meanPoints[arm] = calcNewMean(_meanPoints[arm], points, n);

  _onArmRecording(_armStats[arm], arm);

  printf(
      "Arm %ld got %0.2f points and cost %s. Mean reward %f for %ld "
      "recorded "
      "pulls.\n",
      arm, points, stats._pullBestCost.value().toString().c_str(),
      _meanPoints[arm], _armStats[arm]->timesChosen);
}

std::tuple<std::unique_ptr<AnnealingSchedule>, size_t>
UpperConfidenceBound::chooseArm(RandomProvider& random) {
  std::vector<size_t> goodArms;

  _lock.lock();
  _totalPulls++;

  double largestUCB = 0;
  for (size_t arm = 0; arm < _numArms; ++arm) {
    const double ucb =
        _armStats[arm]->timesRecorded == 0
            ? INFINITY
            : _meanPoints[arm] +
                  sqrt(2 * log(1 + _totalPulls * log(_totalPulls)) /
                       _armStats[arm]->timesRecorded);
    if (ucb > largestUCB) {
      largestUCB = ucb;
      goodArms.clear();
    }
    if (ucb >= largestUCB) {
      goodArms.push_back(arm);
    }
    printf(
        "Pull %ld: Arm %ld has UCB %f for %ld/%ld recorded "
        "pulls.\n",
        _totalPulls, arm, ucb, _armStats[arm]->timesChosen,
        _totalRecordedPulls);
  }

  const size_t i =
      random.fromDistribution<size_t, std::uniform_int_distribution<>>(
          std::uniform_int_distribution<>(0, goodArms.size() - 1));
  const size_t arm = goodArms[i];
  _armStats[arm]->isChosen();
  _lock.unlock();

  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

void UpperConfidenceBound::printStats() const {
  printf("\n");
  for (size_t arm = 0; arm < _numArms; arm++) {
    std::cout << "Arm " << arm << ": " << _armStats[arm]->toString()
              << "\n\tMean points: " << _meanPoints[arm] << std::endl;
  }
}

}  // namespace atlantis::search
