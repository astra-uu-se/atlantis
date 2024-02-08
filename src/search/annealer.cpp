#include "search/annealer.hpp"

namespace atlantis::search {

Annealer::Annealer(const Assignment& assignment, RandomProvider& random,
                   AnnealingSchedule& schedule)
    : _assignment(assignment), _random(random), _schedule(schedule) {
  _statistics.bestCostOfPreviousRound = std::numeric_limits<Int>::max();
  _statistics.bestCostOfThisRound = std::numeric_limits<Int>::max();

  auto numSearchVars = assignment.searchVars().size();
  _requiredMovesPerRound = static_cast<UInt>(
      static_cast<double>(128 * numSearchVars) / std::log2(numSearchVars));
}

bool Annealer::isFinished() const { return _schedule.frozen(); }

void Annealer::nextRound() {
  _schedule.nextRound(_statistics);

  auto previousBest = _statistics.bestCostOfThisRound;
  _statistics = {};
  _statistics.bestCostOfPreviousRound = previousBest;
  _statistics.bestCostOfThisRound = std::numeric_limits<Int>::max();
  _statistics.temperature = _schedule.temperature();

  _attemptedMovesPerRound = 0;
}

bool Annealer::runMonteCarloSimulation() const {
  return _attemptedMovesPerRound < _requiredMovesPerRound;
}

bool Annealer::accept(Int moveCost) {
  Int assignmentCost = evaluate(_assignment.cost());
  Int delta = moveCost - assignmentCost;

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
  } else {
    _statistics.uphillAttemptedMoves++;

    if (std::exp(static_cast<double>(-delta) / _schedule.temperature()) >=
        _random.floatInRange(0.0f, 1.0f)) {
      _statistics.uphillAcceptedMoves++;
      _statistics.acceptedMoves++;
      return true;
    }
  }

  return false;
}

void Annealer::start() {
  _schedule.start(INITIAL_TEMPERATURE);
  _violationWeight = 1;
  _objectiveWeight = 0;
}

}  // namespace atlantis::search