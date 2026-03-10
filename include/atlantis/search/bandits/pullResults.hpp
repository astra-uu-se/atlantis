#pragma once

#include <chrono>

#include "atlantis/search/cost.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::search {

// TODO: Consider making this some sort of record object
class PullResults {
public:
  size_t improvingSolutions = 0;
  std::optional<Cost> _pullBestCost;
  std::optional<std::shared_ptr<RoundStatistics>> _roundStatistics;
  std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

  void submitCost(const Cost& newCost) {
    if (_pullBestCost.has_value() && newCost < _pullBestCost) {
      _pullBestCost = newCost;
      improvingSolutions++;
    }
  }

  void reset(const Cost& baseCost, const std::optional<std::shared_ptr<RoundStatistics>>& roundStatistics) {
    improvingSolutions = 0;
    _pullBestCost = baseCost;
    _roundStatistics = roundStatistics;
    startTime = std::chrono::system_clock::now();
  }
};

}  // namespace atlantis::search
