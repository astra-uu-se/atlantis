#include "atlantis/search/annealer.hpp"

#include <cmath>
#include <limits>

#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::search {

Annealer::Annealer(RandomProvider& random, AnnealingSchedule& schedule,
                   const IAssignment& assignment)
    : _random(random),
      _schedule(schedule),
      _cost(
          std::numeric_limits<Int>::max(),
          assignment.objectiveDirection() == ObjectiveDirection::MINIMIZE
              ? std::numeric_limits<Int>::max()
              : (assignment.objectiveDirection() == ObjectiveDirection::MAXIMIZE
                     ? std::numeric_limits<Int>::min()
                     : 0),
          assignment.objectiveDirection()),
      _statistics(INITIAL_TEMPERATURE),
      _requiredMovesPerRound{static_cast<UInt>(
          static_cast<double>(128 * assignment.searchVars().size()) /
          std::log2(assignment.searchVars().size()))} {}

bool Annealer::isFinished() const { return _schedule.frozen(); }

void Annealer::nextRound() {
  _schedule.nextRound(_statistics);
  _statistics.nextRound(_schedule.temperature());
  _attemptedMovesPerRound = 0;
}

bool Annealer::runMonteCarloSimulation() const {
  return _attemptedMovesPerRound < _requiredMovesPerRound;
}

bool Annealer::accept(Int moveCost) {
  const Int assignmentCost = evaluate(_cost);
  const Int delta = moveCost - assignmentCost;

  _statistics.attemptedMoves++;

  if (delta <= 0) {
    if (delta < 0) {
      _statistics.improvingMoves++;
    }

    if (moveCost < _statistics.bestCostOfThisRound) {
      _statistics.bestCostOfThisRound = moveCost;
    }

    _statistics.acceptedMoves++;
    return true;
  }
  _statistics.uphillAttemptedMoves++;

  if (std::exp(static_cast<double>(-delta) / _schedule.temperature()) >=
      _random.floatInRange(0.0f, 1.0f)) {
    _statistics.uphillAcceptedMoves++;
    _statistics.acceptedMoves++;
    return true;
  }

  return false;
}

void Annealer::start() {
  _schedule.start(INITIAL_TEMPERATURE);
  _violationWeight = 1;
  _objectiveWeight = 0;
}

}  // namespace atlantis::search
