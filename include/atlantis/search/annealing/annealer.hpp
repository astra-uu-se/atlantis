#pragma once

#include <optional>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/search/metaheuristic.hpp"

namespace atlantis::search {

class Assignment;
class RandomProvider;

/**
 * Annealing based on chapter 12 of:
 *
 * P. Van Hentenryck and L. Michel. Constraint-Based Local Search. The MIT
 * Press, 2005.
 */
class Annealer : public MetaHeuristic {
  RandomProvider& _random;
  AnnealingSchedule& _schedule;
  Cost _cost;
  RoundStatistics _statistics;
  UInt _requiredMovesPerRound;
  logging::Logger& _logger;

  UInt _attemptedMovesPerRound{0};

  static constexpr double INITIAL_TEMPERATURE{1.0};

  UInt _violationWeight{1};
  UInt _objectiveWeight{1};

 public:
  Annealer(RandomProvider&, AnnealingSchedule&, const Assignment&,
           logging::Logger&);

  virtual ~Annealer() = default;

  void start() override;

  [[nodiscard]] bool isFinished() const override;

  bool acceptMove(const Cost& cost) override;

 protected:
  void nextRound();

  [[nodiscard]] bool shouldRunRound() const;

  [[nodiscard]] const RoundStatistics& currentRoundStatistics() const;

  virtual bool accept(Int moveCost);

  [[nodiscard]] Int evaluate(const Cost& cost) const;

  void logRoundStatistics();
};

}  // namespace atlantis::search
