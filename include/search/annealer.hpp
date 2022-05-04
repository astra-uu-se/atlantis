#pragma once

#include "move.hpp"
#include "randomProvider.hpp"
#include "search/annealing/annealingSchedule.hpp"

namespace search {

/**
 * Annealing based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class Annealer {
 private:
  const Assignment& _assignment;
  RandomProvider& _random;
  AnnealingSchedule& _schedule;

  UInt _localIterations{0};
  RoundStatistics _statistics{};

 public:
  Annealer(const Assignment& assignment, RandomProvider& random, AnnealingSchedule& schedule)
      : _assignment(assignment),
        _random(random),
        _schedule(schedule) {}

  /**
   * @return True if the annealer has finished.
   */
  [[nodiscard]] bool isFinished() const;

  /**
   * Advance to the next round.
   */
  void nextRound();

  /**
   * @return True whilst more Monte-Carlo simulations need to be run for this
   * round.
   */
  bool runMonteCarloSimulation();

  /**
   * Determine whether @p move should be committed to the assignment.
   *
   * @tparam N The size of the move.
   * @param move The move itself.
   * @return True if @p move should be committed, false otherwise.
   */
  template <unsigned int N>
  bool acceptMove(Move<N> move) {
    // TODO: Weights for the objective and violation.
    Int assignmentCost = _assignment.cost().evaluate(1, 1);
    Int moveCost = move.probe(_assignment).evaluate(1, 1);
    Int delta = moveCost - assignmentCost;

    if (delta <= 0) {
      return true;
    } else {
      _statistics.uphillAttemptedMoves++;

      if (accept(delta)) {
        _statistics.uphillAcceptedMoves++;
        return true;
      }
    }

    return false;
  }

 private:
  [[nodiscard]] bool accept(Int delta) const;
};

}  // namespace search