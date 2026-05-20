#include "atlantis/search/bandits/ThompsonSamplingBernoulli.hpp"

#include "../../../include/atlantis/search/bandits/armSelector.hpp"
#include "atlantis/search/bandits/costRewardController.hpp"

namespace atlantis::search {

ThompsonSamplingBernoulli::ThompsonSamplingBernoulli(
const std::shared_ptr<AnnealingScheduleFactory>& annealingScheduleFactory,
  const std::function<void(std::shared_ptr<ArmStats>, size_t)>& onArmRecording)
    : ArmSelector(annealingScheduleFactory, onArmRecording) {
  _rewardController = std::make_unique<CostRewardController>(_armStats);
  _alpha = std::vector(_numArms, 1.0);
  _beta = std::vector(_numArms, 1.0);
}

void ThompsonSamplingBernoulli::recordArmStats(const size_t arm, const PullResults& stats) {
  std::lock_guard lock(_lock);
  const double points = _rewardController->getReward(arm, stats);
  _alpha[arm] += points;
  _beta[arm] += 1 - points;
  _totalRecordedPulls++;

  _onArmRecording(_armStats[arm], arm);
}


std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> ThompsonSamplingBernoulli::chooseArm(RandomProvider& random) {
  size_t arm = 0;

  std::lock_guard lock(_lock);

  if (_totalPulls < _numArms) {
    arm = _totalPulls;
  }
  else {
    double sample = -1;
    for (size_t i = 0; i < _numArms; i++) {
      const double x = random.fromDistribution<double, std::gamma_distribution<>>(
          std::gamma_distribution(_alpha[i], 1.0));
      const double y = random.fromDistribution<double, std::gamma_distribution<>>(
          std::gamma_distribution(_beta[i], 1.0));
      const double mean = x / (x + y);
      // printf("\tSampled mean %ld for arm %ld from Gamma distribution with params [%.1f, 1/%.1f].\n", mean, i, _alpha[i], _beta[i]);
      // NOTE: this design gives a slight bias to arms with lower indices.
      if (mean > sample) {
        sample = mean;
        arm = i;
      }
    }
  }

  _totalPulls++;
  _armStats[arm]->timesChosen++;

  return std::make_tuple(_annealingScheduleFactory->create(arm), arm);
}

void ThompsonSamplingBernoulli::printStats() const {
  printf("\n");
  for (size_t arm = 0; arm < _numArms; arm++) {
    std::cout << "Arm " << arm << ": " << _armStats[arm]->toString() <<
      "\n\talpha:   " << _alpha[arm] <<
      "\n\tbeta:    " << _beta[arm] << std::endl;
  }
}

}  // namespace atlantis::search
