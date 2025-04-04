#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "atlantis/search/cost.hpp"

namespace atlantis::search {

class IAssignment;
class RandomProvider;

/**
 * Annealing based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class Annealer {
  RandomProvider& _random;
  AnnealingSchedule& _schedule;
  Cost _cost;

  UInt _requiredMovesPerRound{0};
  UInt _attemptedMovesPerRound{0};
  RoundStatistics _statistics;

  const double INITIAL_TEMPERATURE = 1.0;

  UInt _violationWeight{1};
  UInt _objectiveWeight{1};

 public:
  Annealer(RandomProvider&, AnnealingSchedule&, const IAssignment&);

  virtual ~Annealer() = default;

  void start();

  [[nodiscard]] bool isFinished() const;

  void nextRound();

  [[nodiscard]] bool runMonteCarloSimulation() const;

  bool acceptMove(const Cost& cost) {
    _attemptedMovesPerRound++;

    return accept(evaluate(cost));
  }

  [[nodiscard]] const RoundStatistics& currentRoundStatistics() const {
    return _statistics;
  }

 protected:
  virtual bool accept(Int moveCost);

  [[nodiscard]] Int evaluate(const Cost& cost) const {
    return cost.evaluate(_violationWeight, _objectiveWeight);
  }
};

}  // namespace atlantis::search
