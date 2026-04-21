#include <cmath>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/bandits/OCUCBn.hpp"
#include "atlantis/search/bandits/costRewardController.hpp"
#include "atlantis/search/bandits/isImprovementsRewardController.hpp"
#include "atlantis/search/bandits/upperConfidenceBound.hpp"

namespace atlantis::search {

OCUCBn::OCUCBn(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _rewardController = std::make_unique<CostRewardController>(_armStats);
  _meanPoints = std::vector(_numArms, 0.0);
}

static double calcNewMean(const double oldMean, const double newValue,
                          const double newN) {
  return (oldMean * (newN - 1) + newValue) / newN;
}

double OCUCBn::b(const size_t i, const size_t t) const {
  double sum = 0;
  for (size_t j = 0; j < _numArms; j++) {
    const double alt1 = _armStats[i]->timesRecorded;
    const double alt2 = pow(_armStats[j]->timesRecorded, _rho) * pow(_armStats[j]->timesRecorded, 1-_rho);
    sum += std::min(alt1, alt2);
  }

  const double firstMax = std::max(exp(1), log(t));
  return std::max(firstMax, t*log(t)/sum);
}

double OCUCBn::gamma(const size_t i, const size_t t) const {
  if (t == 0) return INFINITY;

  return _meanPoints[i] + sqrt( 2 * _eta * b(i,t) / _armStats[i]->timesRecorded );
}

void OCUCBn::recordArmStats(const size_t arm, const PullResults& stats) {
  std::lock_guard lock(_lock);
  _totalRecordedPulls++;

  const double points = _rewardController->getReward(arm, stats);
  _meanPoints[arm] = calcNewMean(_meanPoints[arm], points, _armStats[arm]->timesRecorded);

  printf(
      "Arm %ld got %0.2f points and cost %s. Mean reward %f for %ld "
      "recorded pulls.\n",
      arm, points, stats._pullBestCost.value().toString().c_str(),
      _meanPoints[arm], _armStats[arm]->timesChosen);
}

std::tuple<std::unique_ptr<AnnealingSchedule>, size_t>
OCUCBn::chooseArm(RandomProvider& random) {
  std::vector<size_t> goodArms;

  _lock.lock();

  if (_totalPulls < _numArms) {
    goodArms.push_back(_totalPulls);
  } else {
    double largestUCB = 0;
    for (size_t arm = 0; arm < _numArms; ++arm) {
      const double ucb = gamma(arm, _totalRecordedPulls);
      if (ucb > largestUCB) {
        largestUCB = ucb;
        goodArms.clear();
      }
      if (ucb >= largestUCB) {
        goodArms.push_back(arm);
      }
      printf(
          "Pull %ld: Arm %ld has UCB %f for %ld/%ld recorded "
          "pulls with mean %.3f.\n",
          _totalPulls, arm, ucb, _armStats[arm]->timesChosen,
          _totalRecordedPulls, _meanPoints[arm]);
    }
  }

  const size_t i =
      random.fromDistribution<size_t, std::uniform_int_distribution<>>(
          std::uniform_int_distribution<>(0, goodArms.size() - 1));
  const size_t arm = goodArms[i];

  _totalPulls++;
  _armStats[arm]->isChosen();
  _lock.unlock();

  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

void OCUCBn::printStats() const {
  printf("\n");
  for (size_t arm = 0; arm < _numArms; arm++) {
    std::cout << "Arm " << arm << ": " << _armStats[arm]->toString()
              << "\n\tMean points: " << _meanPoints[arm] << std::endl;
  }
}

}  // namespace atlantis::search
