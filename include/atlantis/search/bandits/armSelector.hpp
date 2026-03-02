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
  double runTime = 0;
  std::vector<Int> rewards;

double addResult(const PullResults& result) {
  Int reward = result.improvingSolutions;
  rewards.push_back(reward);

  runTime +=
    std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - result.startTime)
        .count();

  return static_cast<double>(reward);
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

  virtual void recordArmStats(size_t arm, const PullResults &stats) = 0;

  virtual std::tuple<std::unique_ptr<AnnealingSchedule>, size_t> chooseArm(RandomProvider& random) = 0;

  void printStats() const {
    for (size_t arm = 0; arm < _numArms; arm++) {
      const auto a = _armStats[arm];
      printf("Arm %ld was chosen %ld times with %.1f avg time (ms).\n", arm, a.timesChosen, a.runTime / a.timesChosen / 1000);
    }
  }
};

}  // namespace atlantis::search