#include "atlantis/search/bandits/KullbackLeiblerUpperConfidenceBound.hpp"

// #include "atlantis/search/bandits/armSelector.hpp"
#include <iostream>

#include "atlantis/search/annealing/types.hpp"
#include "atlantis/search/bandits/costRewardController.hpp"

namespace atlantis::search {

KullbackLeiblerUpperConfidenceBound::KullbackLeiblerUpperConfidenceBound(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _rewardController = std::make_unique<CostRewardController>(_armStats);

  _meanPoints = std::vector(_numArms, 0.0);
}

static double calcNewMean(const double oldMean, const double newValue, const double newN) {
  return (oldMean * (newN - 1) + newValue) / newN;
}

static double findMaxQ(const size_t n, const double p, const size_t t) {
  // This is an const limits how far the q-value will be optimized.
  // 5 should be more than enough and still plenty fast.
  constexpr int newtonSteps = 5;

  // If n == 0 no data has yet been recorded for the arm, so we assume the highest bound.
  // p == 1 => q == 1 since q is in the range [p, 1].
  if (p == 1 || n == 0) return 1;

  // g = n * d(p,q) - upperBound
  // d(p,q) is the Kullback-Leibler divergence, in this case for a Bernoulli distribution.
  constexpr double c = 0; // The paper recommends this be set to 0 in practice.
  const double upperBound = log(t) + c * log(log(t));
  // Assuming 0 <= p < 1 && 0 <= q <= 1
  const auto g = [&](const double q) { return p == 0 ? n*log(1/(1-q))-upperBound : n * (p * log(p/q) +(1-p) * log((1-p)/(1-q))) - upperBound; };
  const auto dg = [&](const double q) { return n * (-p/q + (1-p)/(1-q)); };

  if (g(1) <= 0) {
    return 1;
  }

  double hi = p;
  while (g(hi) <= 0.0) {
    hi = std::midpoint(hi, 1.0);
  }

  double q = hi;
  for (int i = 0; i < newtonSteps; i++) {
    const double step = g(q) / dg(q);
    q -= step;
    q = std::clamp(q, p, hi);
    if (std::abs(step) < 1e-10) break;
  }

  return q;
}

void KullbackLeiblerUpperConfidenceBound::recordArmStats(const size_t arm, const PullResults& stats) {
  std::lock_guard lock(_lock);
  _totalRecordedPulls++;

  const double points = _rewardController->getReward(arm, stats);

  const size_t n = _armStats[arm]->timesRecorded;
  // _meanPoints[arm] *= 0.99;
  _meanPoints[arm] = calcNewMean(_meanPoints[arm], points, n);

  printf("Arm %ld got %0.2f points and cost %s. Mean reward %f for %ld recorded pulls.\n",
    arm, points, stats._pullBestCost.value().toString().c_str(),
    _meanPoints[arm], _armStats[arm]->timesChosen);
}

std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> KullbackLeiblerUpperConfidenceBound::chooseArm(RandomProvider& random) {
  std::vector<size_t> goodArms;

  _lock.lock();
    _totalPulls++;

    double largestUCB = 0;
    for (size_t arm = 0; arm < _numArms; ++arm) {
      const double p = _meanPoints[arm];
      const double ucb = findMaxQ(_armStats[arm]->timesChosen, p, _totalRecordedPulls);
      if (ucb > largestUCB) {
        largestUCB = ucb;
        goodArms.clear();
      }
      if (ucb >= largestUCB) {
        goodArms.push_back(arm);
      }
      // printf("  Pull %ld: Arm %ld has KL-UCB q=%f for p=%f for %ld/%ld recorded pulls.\n", _totalPulls, arm, ucb, p, _armStats[arm].timesChosen, _totalRecordedPulls);
    }

  const size_t i = random.fromDistribution<size_t, std::uniform_int_distribution<>>(
      std::uniform_int_distribution<>(0, goodArms.size()-1));
  const size_t arm = goodArms[i];

  _armStats[arm]->isChosen();
  _lock.unlock();

  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

void KullbackLeiblerUpperConfidenceBound::printStats() const {
  printf("\n");
  for (size_t arm = 0; arm < _numArms; arm++) {
    std::cout << "Arm " << arm << ": " << _armStats[arm]->toString() <<
      "\n\tMean points: " << _meanPoints[arm] << std::endl;
  }
}

}  // namespace atlantis::search
