#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "atlantis/search/cost.hpp"

namespace atlantis::search {

class Assignment;
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
  RoundStatistics _statistics;
  UInt _requiredMovesPerRound;

  UInt _attemptedMovesPerRound{0};

  static constexpr double INITIAL_TEMPERATURE{1.0};

  UInt _violationWeight{1};
  UInt _objectiveWeight{1};

 public:
  Annealer(RandomProvider&, AnnealingSchedule&, const Assignment&);

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
