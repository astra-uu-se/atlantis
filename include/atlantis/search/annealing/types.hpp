#pragma once

#include "atlantis/search/cost.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

struct RoundStatistics {
  UInt uphillAttemptedMoves{0};
  UInt uphillAcceptedMoves{0};

  UInt attemptedMoves{0};
  UInt acceptedMoves{0};
  UInt improvingMoves{0};
  UInt rounds{0};

  Cost bestCostOfPreviousRound;
  Cost bestCostOfThisRound;

  double temperature;

  explicit RoundStatistics(double temperature) : temperature(temperature) {}

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
    rounds++;
    bestCostOfPreviousRound = bestCostOfThisRound;
    bestCostOfThisRound = Cost();
    temperature = temp;
  }
};

}  // namespace atlantis::search
