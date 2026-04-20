#pragma once

#include <queue>

#include "armStats.hpp"
#include "pullResults.hpp"
#include "rewardController.hpp"

namespace atlantis::search {

class CostAverage {
  bool _hasViolation;
  bool _hasObjective;
  double _violation;
  double _objective;

  size_t _size;
  std::queue<Cost> _history;

 public:
  explicit CostAverage(const Cost& initialCost, const size_t size)
      : _hasViolation(initialCost.hasViolation()),
        _hasObjective(initialCost.hasObjective()),
        _size(size) {
    if (_hasViolation) _violation = initialCost.violation();
    if (_hasObjective) _objective = initialCost.objective();

    _history.push(initialCost);
  }

  void addCost(const Cost& newCost) {
    if (_history.size() <= _size) {
      if (_hasObjective) {
        _objective = (_objective * _history.size() + newCost.objective()) /
                     (_history.size() + 1);
      }
      if (_hasViolation) {
        _violation = (_violation * _history.size() + newCost.violation()) /
                     (_history.size() + 1);
      }

      _history.push(newCost);
      return;
    }

    const auto remove = _history.front();

    if (_hasObjective) {
      _objective =
          (_objective * _size - remove.objective() + newCost.objective()) /
          _size;
    }
    if (_hasViolation) {
      _violation =
          (_violation * _size - remove.violation() + newCost.violation()) /
          _size;
    }

    _history.pop();
    _history.push(newCost);
  }

  Cost cost() const {
    if (!_hasObjective) return Cost(static_cast<Int>(_violation));
    if (!_hasViolation) return Cost(static_cast<Int>(_objective), true);
    return Cost(static_cast<Int>(_violation), static_cast<Int>(_objective),
                true);
  }
};

class CostRewardController : public RewardController {
  std::optional<CostAverage> _costAverage;
  std::vector<std::shared_ptr<ArmStats>> _armStats;

 public:
  explicit CostRewardController(
      const std::vector<std::shared_ptr<ArmStats>>& armStats)
      : _armStats(armStats) {}

  [[nodiscard]] double getReward(const size_t arm,
                                 const PullResults& results) override {
    if (!results._pullBestCost.has_value()) {
      throw std::runtime_error("Results doesn't contain reward value!");
    }

    _armStats[arm]->addResult(results);
    const Cost reward = results._pullBestCost.value();

    double points;

    if (!_costAverage.has_value()) {
      _costAverage = CostAverage(reward, 10);
      points = 1;
    } else {
      if (reward <= _costAverage->cost()) {
        points = 1;
      } else {
        points = 0;
      }

      _costAverage->addCost(reward);
    }

    printf("Arm %ld got cost %s. Global is %s, so it gets %.0f point.\n", arm,
           reward.toString().c_str(), _costAverage->cost().toString().c_str(),
           points);

    return points;
  }
};

}  // namespace atlantis::search
