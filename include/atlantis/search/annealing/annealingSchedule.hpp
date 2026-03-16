#pragma once

#include <memory>

namespace atlantis::search {

struct RoundStatistics;

class AnnealingSchedule {
 public:
  virtual ~AnnealingSchedule() = default;

    [[nodiscard]] virtual std::unique_ptr<AnnealingSchedule> clone() const = 0;

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
  virtual void nextRound(
      const std::shared_ptr<RoundStatistics>& statistics) = 0;

  /**
   * @return The current temperature.
   */
  [[nodiscard]] virtual double temperature() const = 0;

  /**
   * @return True if the schedule has completed, false otherwise.
   */
  [[nodiscard]] virtual bool frozen() const = 0;
};

}  // namespace atlantis::search
