#pragma once
#include <array>
#include <vector>

#include "atlantis/search/annealing/annealingScheduleFactory.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "pullResults.hpp"

namespace atlantis::search {

class ArmStats {
public:
  size_t timesChosen = 0;
  double mean = 0;
  std::vector<Int> rewards;

  // Variables for Thompson sampling
  double mu = 0;
  double sigmaSquared = 0;

void addResult(const Int result) {
  rewards.push_back(result);

  mean = std::accumulate(rewards.begin(), rewards.end(), 0)
          / rewards.size();

  // If Thompson, update mean.
}
};

enum class BanditAlgorithm: unsigned char { ETC, UCB, Thompson };
constexpr std::array<std::string_view, 3> banditAlgorithmNames = {"explore-then-commit", "UCB", "Thompson sampling"};

class ArmSelector {
protected:
  mutable std::mutex _lock;
  size_t _numArms = 0;
  std::vector<ArmStats> _armStats;
  std::shared_ptr<AnnealingScheduleFactory>
      _annealingScheduleFactory;

public:

  explicit ArmSelector(
     const std::shared_ptr<AnnealingScheduleFactory> &annealingScheduleFactory) :
    _numArms(annealingScheduleFactory->armCount()),
    _annealingScheduleFactory(annealingScheduleFactory) {
    _armStats = std::vector<ArmStats>(_numArms);
  }

  virtual ~ArmSelector() = default;

  virtual void recordArmStats(size_t arm, const std::unique_ptr<PullResults> &stats) = 0;

  virtual std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) = 0;
};

}  // namespace atlantis::search