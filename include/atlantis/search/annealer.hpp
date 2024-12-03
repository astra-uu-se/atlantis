#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/move.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::search {

/**
 * Annealing based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class Annealer {
 private:
  Assignment& _assignment;
  RandomProvider& _random;
  AnnealingSchedule& _schedule;

  UInt _requiredMovesPerRound{0};
  UInt _attemptedMovesPerRound{0};
  RoundStatistics _statistics{};

  const double INITIAL_TEMPERATURE = 1.0;

  UInt _violationWeight{1};
  UInt _objectiveWeight{1};

 public:
  Annealer(Assignment&, RandomProvider&, AnnealingSchedule&);

  virtual ~Annealer() = default;

  /**
   * Start the annealing process.
   */
  void start();

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
  [[nodiscard]] bool runMonteCarloSimulation() const;

  /**
   * Determine whether @p move should be committed to the assignment.
   *
   * @tparam N The size of the move.
   * @param move The move itself.
   * @return True if @p move should be committed, false otherwise.
   */
  bool acceptMove(Move move) {
    _attemptedMovesPerRound++;

    Int moveCost = evaluate(move.probe(_assignment));
    return accept(moveCost);
  }

  [[nodiscard]] const RoundStatistics& currentRoundStatistics() {
    return _statistics;
  }

 protected:
  virtual bool accept(Int moveCost);

  [[nodiscard]] inline Int evaluate(Cost cost) const {
    return cost.evaluate(_violationWeight, _objectiveWeight);
  }
};

}  // namespace atlantis::search
