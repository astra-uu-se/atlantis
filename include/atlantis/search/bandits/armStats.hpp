#pragma once

#include <memory>
#include <chrono>

#include "pullResults.hpp"

namespace atlantis::search {
class ArmStats {
  // Stats for evaluation
  double _runTime = 0;
  double _meanProbes = 0;
  double _meanMoves = 0;
  double _meanImprovingMoves = 0;
  double _meanRounds = 0;

public:
  size_t timesChosen = 0;
  size_t timesRecorded = 0;
  // Initialized as the worst possible cost
  std::optional<Cost> bestCost;

  explicit ArmStats() {}

  void addResult(const PullResults& result) {
    timesRecorded++;

    _runTime +=
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now() - result.startTime)
          .count();

    if (bestCost.has_value() && result._pullBestCost.has_value()) {
      if (!(result._pullBestCost.value() < bestCost.value())) {
      } else {
        bestCost = result._pullBestCost.value();
      }
    } else if (!bestCost.has_value()) {
      bestCost = result._pullBestCost;
    }

    _meanProbes = (_meanProbes * (timesRecorded - 1) + result._roundStatistics.value()->attemptedMoves) / timesRecorded;
    _meanMoves = (_meanMoves * (timesRecorded - 1) + result._roundStatistics.value()->acceptedMoves) / timesRecorded;
    _meanImprovingMoves = (_meanImprovingMoves * (timesRecorded - 1) + result._roundStatistics.value()->improvingMoves) / timesRecorded;
    _meanRounds = (_meanRounds * (timesRecorded - 1) + result._roundStatistics.value()->rounds) / timesRecorded;
  }

  void isChosen() {
    timesChosen++;
  }

  [[nodiscard]] std::string toString() const {
    auto c = bestCost.has_value() ? bestCost.value().toString() : "-";
    return "Chosen " + std::to_string(timesChosen) + " times and recorded " +
           std::to_string(timesRecorded) + " times with best cost " + c +
           " and averages: \n\tRuntime: " +
           std::to_string(_runTime / timesRecorded / 1000) +
           "ms\n\tProbes:  " + std::to_string(static_cast<Int>(_meanProbes)) +
           "\n\tMoves:   " + std::to_string(static_cast<Int>(_meanMoves)) +
           "\n\tImproving moves: " +
           std::to_string(static_cast<Int>(_meanImprovingMoves)) +
           "\n\tRounds:  " + std::to_string(static_cast<Int>(_meanRounds));
  }
};

}  // namespace atlantis::search
