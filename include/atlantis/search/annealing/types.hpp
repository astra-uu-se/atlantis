#pragma once

#include <limits>

#include "atlantis/types.hpp"

namespace atlantis::search {

struct RoundStatistics {
  UInt uphillAttemptedMoves{0};
  UInt uphillAcceptedMoves{0};

  UInt attemptedMoves{0};
  UInt acceptedMoves{0};
  UInt improvingMoves{0};

  Int bestCostOfPreviousRound{std::numeric_limits<Int>::max()};
  Int bestCostOfThisRound{std::numeric_limits<Int>::max()};

  double temperature;

  explicit RoundStatistics(double temperature) :
  temperature(temperature) {}

  explicit RoundStatistics() : RoundStatistics(1.0) {}

  [[nodiscard]] double uphillAcceptanceRatio() const noexcept {
    return static_cast<double>(uphillAcceptedMoves) /
           static_cast<double>(uphillAttemptedMoves);
  }

  [[nodiscard]] double moveAcceptanceRatio() const noexcept {
    return static_cast<double>(acceptedMoves) /
           static_cast<double>(attemptedMoves);
  }

  [[nodiscard]] double improvingMoveRatio() const noexcept {
    return static_cast<double>(improvingMoves) /
           static_cast<double>(attemptedMoves);
  }

  [[nodiscard]] bool roundImprovedOnPrevious() const noexcept {
    return bestCostOfThisRound < bestCostOfPreviousRound;
  }

  void nextRound(double temp) noexcept {
    bestCostOfPreviousRound = bestCostOfThisRound;
    bestCostOfThisRound = std::numeric_limits<Int>::max();
    temperature = temp;
  }
};

}  // namespace atlantis::search
