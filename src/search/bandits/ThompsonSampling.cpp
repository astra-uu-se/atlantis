#include "atlantis/search/bandits/ThompsonSampling.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::search {

ThompsonSampling::ThompsonSampling(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _alpha = std::vector(_numArms, 1.0);
  _beta = std::vector(_numArms, 0.001);

  _meanProbes = std::vector(_numArms, 0.0);
  _meanMoves = std::vector(_numArms, 0.0);
  _meanImprovingMoves = std::vector(_numArms, 0.0);
  _meanRounds = std::vector(_numArms, 0.0);
  _meanRewards = std::vector(_numArms, 0.0);
}


static double calcNewMean(const double oldMean, const double newValue, const double newN) {
  return (oldMean * (newN - 1) + newValue) / newN;
}


void ThompsonSampling::recordArmStats(const size_t arm,
                    const PullResults& stats) {
  printf("Arm %ld got result %ld and cost %s.\n", arm, stats.improvingSolutions, stats._pullBestCost.value().toString().c_str());

  std::lock_guard lock(_lock);
  const double reward = _armStats[arm].addResult(stats);
  _alpha[arm] += reward;
  _beta[arm]++;
  _totalPulls++;

  const size_t n = _armStats[arm].timesChosen;
  _meanProbes[arm] = calcNewMean(_meanProbes[arm], stats._roundStatistics.value()->attemptedMoves, n);
  _meanMoves[arm] = calcNewMean(_meanMoves[arm], stats._roundStatistics.value()->acceptedMoves, n);
  _meanImprovingMoves[arm] = calcNewMean(_meanImprovingMoves[arm], stats._roundStatistics.value()->improvingMoves, n);
  _meanRounds[arm] = calcNewMean(_meanRounds[arm], stats._roundStatistics.value()->rounds, n);
  _meanRewards[arm] = calcNewMean(_meanRewards[arm], reward, n);
}


std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ThompsonSampling::chooseArm(RandomProvider& random) {
  size_t arm = 0;
  Int sample = INT_MIN;

  _lock.lock();

  for (size_t i = 0; i < _numArms; i++) {
    const Int mean = random.fromDistribution<Int, std::gamma_distribution<>>(
        std::gamma_distribution(_alpha[i], 1 / _beta[i]));
    // printf("\tSampled mean %ld for arm %ld from Gamma distribution with params [%.1f, 1/%.1f].\n", mean, i, _alpha[i], _beta[i]);
    // Note that this design gives a slight bias to arms with lower indices.
    if (mean > sample) {
      sample = mean;
      arm = i;
    }
  }

  _armStats[arm].timesChosen++;

  _lock.unlock();

  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

void ThompsonSampling::printStats() const {
  printf("\n");
  for (size_t arm = 0; arm < _numArms; arm++) {
    const auto a = _armStats[arm];
    printf("Arm %ld was chosen %ld times with %.1f avg time (ms).\n"
           "\tAverages: probes %.f, moves %.f, improving moves %.f, rounds %.f, rewards %.f. \n",
           arm, a.timesChosen, a.runTime / a.timesChosen / 1000,
           _meanProbes[arm], _meanMoves[arm], _meanImprovingMoves[arm], _meanRounds[arm], _meanRewards[arm]);
  }
}

}  // namespace atlantis::search
