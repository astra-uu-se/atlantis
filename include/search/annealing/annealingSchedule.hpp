#pragma once

#include "core/types.hpp"
namespace search {

struct RoundStatistics {
  UInt uphillAttemptedMoves;
  UInt uphillAcceptedMoves;

  [[nodiscard]] inline double uphillAcceptanceRatio() const noexcept {
    return static_cast<double>(uphillAcceptedMoves) / static_cast<double>(uphillAttemptedMoves);
  }
};

class AnnealingSchedule {
 public:
  virtual ~AnnealingSchedule() = default;

  /**
   * Start the annealing schedule. This should reset the internal state of the
   * schedule and start anew. Annealing combinators will use this when switching
   * between schedules.
   *
   * @param initialTemperature The temperature to start the schedule at.
   */
  virtual void start(double initialTemperature) = 0;

  /**
   * Indicate to the schedule a round has finished, and the next round should
   * start.
   *
   * @param statistics
   */
  virtual void nextRound(const RoundStatistics& statistics) = 0;

  /**
   * @return The current temperature.
   */
  virtual double temperature() = 0;

  /**
   * @return The number of Monte-Carlo simulations to perform per round.
   */
  virtual UInt numberOfMonteCarloSimulations() = 0;

  /**
   * @return True if the schedule has completed, false otherwise.
   */
  virtual bool frozen() = 0;
};

}