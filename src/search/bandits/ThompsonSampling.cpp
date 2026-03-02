#include "atlantis/search/bandits/ThompsonSampling.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"

namespace atlantis::search {

ThompsonSampling::ThompsonSampling(
    const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory)
    : ArmSelector(annealingScheduleFactory) {
  _alpha = std::vector<double>(_numArms, 1.0);
  _beta = std::vector<double>(_numArms, 0.001);
  printf("Using bandit algorithm Thompson sampling.\n");
}


void ThompsonSampling::recordArmStats(const size_t arm,
                    const PullResults& stats) {
  printf("Arm %ld got result %ld and cost %s.\n", arm, stats.improvingSolutions, stats._pullBestCost.value().toString().c_str());

  std::lock_guard lock(_lock);
  const double reward = _armStats[arm].addResult(stats);
  _alpha[arm] += reward;
  _beta[arm]++;
  _totalPulls++;
}


std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ThompsonSampling::chooseArm(RandomProvider& random) {
  size_t arm = 0;
  Int sample = INT_MIN;

  _lock.lock();

  for (size_t i = 0; i < _numArms; i++) {
    const Int mean = random.fromDistribution<Int, std::gamma_distribution<>>(std::gamma_distribution(_alpha[i], 1 / _beta[i]));
    printf("\tSampled mean %ld for arm %ld from Gamma distribution with params [%.1f, 1/%.1f].\n", mean, i, _alpha[i], _beta[i]);
    // Note that this design gives a slight bias to arms with lower indices.
    if (mean > sample) {
      sample = mean;
      arm = i;
    }
  }

  _armStats[arm].timesChosen++;

  _lock.unlock();

  printf("  Choosing arm %ld. Times chosen [%ld, %ld].\n", arm, _armStats[0].timesChosen, _armStats[1].timesChosen);
  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

}  // namespace atlantis::search
