#include "atlantis/search/bandits/KullbackLeiblerUpperConfidenceBound.hpp"

// #include "atlantis/search/bandits/armSelector.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::search {

KullbackLeiblerUpperConfidenceBound::KullbackLeiblerUpperConfidenceBound(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _meanRewards = std::vector(_numArms, 0.0);

  // Stats stuff - not necessary for the solver.
  _meanProbes = std::vector(_numArms, 0.0);
  _meanMoves = std::vector(_numArms, 0.0);
  _meanImprovingMoves = std::vector(_numArms, 0.0);
  _meanRounds = std::vector(_numArms, 0.0);
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

  double reward = _armStats[arm].addResult(stats);

  // Really bad solution to make a Bernoulli-ish reward
  // TODO: Come up with something better
  if (stats._pullBestCost.value().objective() < _bestCost) {
    printf("Arm %ld found new best solution with cost %ld. Previous best %ld.\n", arm, stats._pullBestCost.value().objective(), _bestCost);
    _bestCost = stats._pullBestCost.value().objective();
    reward = 1;
  }
  else {
    reward = 0;
  }

  _armStats[arm].timesChosen++;
  const size_t n = _armStats[arm].timesChosen;

  _meanRewards[arm] = calcNewMean(_meanRewards[arm], reward, n);

  // Stats stuff - not necessary for the solver.
  _meanProbes[arm] = calcNewMean(_meanProbes[arm], stats._roundStatistics.value()->attemptedMoves, n);
  _meanMoves[arm] = calcNewMean(_meanMoves[arm], stats._roundStatistics.value()->acceptedMoves, n);
  _meanImprovingMoves[arm] = calcNewMean(_meanImprovingMoves[arm], stats._roundStatistics.value()->improvingMoves, n);
  _meanRounds[arm] = calcNewMean(_meanRounds[arm], stats._roundStatistics.value()->rounds, n);

  printf("Arm %ld got reward %0.2f and cost %s. Mean reward %f for %ld recorded pulls.\n",
    arm, reward, stats._pullBestCost.value().toString().c_str(),
    _meanRewards[arm], _armStats[arm].timesChosen);
}

std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> KullbackLeiblerUpperConfidenceBound::chooseArm(RandomProvider& random) {
  std::vector<size_t> goodArms;

  _lock.lock();
    _totalPulls++;

    double largestUCB = 0;
    for (size_t arm = 0; arm < _numArms; ++arm) {
      // const double ucb = _UCBs[arm];
      const double p = _meanRewards[arm];
      const double ucb = findMaxQ(_armStats[arm].timesChosen, p, _totalRecordedPulls);
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

  _lock.unlock();

  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

void KullbackLeiblerUpperConfidenceBound::printStats() const {
  printf("\n");
  for (size_t arm = 0; arm < _numArms; arm++) {
    const auto a = _armStats[arm];
    printf("Arm %ld was chosen %ld times with %.1f avg time (ms). Total reward %f.\n"
           "\tAverages: probes %.f, moves %.f, improving moves %.f, rounds %.f, rewards %f. \n",
           arm, a.timesChosen, a.runTime / a.timesChosen / 1000, _meanRewards[arm] * a.timesChosen,
           _meanProbes[arm], _meanMoves[arm], _meanImprovingMoves[arm], _meanRounds[arm], _meanRewards[arm]);
  }
}

}  // namespace atlantis::search
