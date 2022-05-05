#include "search/annealer.hpp"

#include <limits>

search::Annealer::Annealer(const search::Assignment& assignment,
                           search::RandomProvider& random,
                           search::AnnealingSchedule& schedule)
    : _assignment(assignment), _random(random), _schedule(schedule) {
  _statistics.bestCostOfPreviousRound = std::numeric_limits<Int>::max();
  _statistics.bestCostOfThisRound = std::numeric_limits<Int>::max();
}

bool search::Annealer::isFinished() const { return _schedule.frozen(); }

void search::Annealer::nextRound() {
  _schedule.nextRound(_statistics);

  auto previousBest = _statistics.bestCostOfThisRound;
  _statistics = {};
  _statistics.bestCostOfPreviousRound = previousBest;

  _localIterations = 0;
}

bool search::Annealer::runMonteCarloSimulation() {
  ++_localIterations;
  return _localIterations < _schedule.numberOfMonteCarloSimulations();
}

bool search::Annealer::accept(Int moveCost) {
  Int assignmentCost = evaluate(_assignment.cost());
  Int delta = moveCost - assignmentCost;

  if (delta <= 0) {
    if (moveCost < _statistics.bestCostOfThisRound) {
      _statistics.bestCostOfThisRound = moveCost;
    }

    return true;
  } else {
    _statistics.uphillAttemptedMoves++;

    if (std::exp(static_cast<double>(-delta) / _schedule.temperature()) >=
        _random.floatInRange(0.0f, 1.0f)) {
      _statistics.uphillAcceptedMoves++;
      return true;
    }
  }

  return false;
}
