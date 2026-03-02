#pragma once

#include <chrono>

#include "../cost.hpp"

namespace atlantis::search {

// TODO: Consider making this some sort of record object
class PullResults {
public:
  size_t improvingSolutions = 0;
  std::optional<Cost> _pullBestCost;
  std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

  void submitCost(const Cost& newCost) {
    if (_pullBestCost.has_value() && newCost < _pullBestCost) {
      _pullBestCost = newCost;
      improvingSolutions++;
    }
  }

  void reset(const Cost& baseCost) {
    improvingSolutions = 0;
    _pullBestCost = baseCost;
    startTime = std::chrono::system_clock::now();
  }
};

}  // namespace atlantis::search
