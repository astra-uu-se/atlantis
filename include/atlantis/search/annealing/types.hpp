#pragma once

#include "atlantis/types.hpp"

namespace atlantis::search {

struct RoundStatistics {
  UInt uphillAttemptedMoves;
  UInt uphillAcceptedMoves;

  UInt attemptedMoves;
  UInt acceptedMoves;
  UInt improvingMoves;

  Int bestCostOfPreviousRound;
  Int bestCostOfThisRound;

  double temperature;

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
};

}  // namespace atlantis::search
