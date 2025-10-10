#include "atlantis/search/annealing/annealer.hpp"

#include <cmath>
#include <limits>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"

namespace atlantis::search {

static UInt reqMovesPerRound(size_t numSearchVars) {
  return static_cast<UInt>(static_cast<double>(128 * numSearchVars) /
                           std::log2(numSearchVars));
}

Annealer::Annealer(RandomProvider& random,
                   std::unique_ptr<AnnealingSchedule>&& schedule,
                   const Assignment& assignment)
    : _random(random),
      _schedule(std::move(schedule)),
      _cost(assignment),
      _statistics(INITIAL_TEMPERATURE),
      _requiredMovesPerRound(reqMovesPerRound(assignment.searchVars().size())) {}

bool Annealer::isFinished() const { return _schedule->frozen(); }

void Annealer::nextRound() {
  _schedule->nextRound(_statistics);
  _statistics.nextRound(_schedule->temperature());
  _attemptedMovesPerRound = 0;
}

bool Annealer::shouldRunRound() const {
  return _attemptedMovesPerRound < _requiredMovesPerRound;
}

bool Annealer::acceptMove(const Cost& cost) {
  ++_attemptedMovesPerRound;

  const bool ret = accept(evaluate(cost));

  if (!shouldRunRound()) {
    nextRound();
  }
  return ret;
}

void Annealer::logRoundStatistics(logging::Logger& logger) {
  logger.trace("Accepted over attempted moves: {:d} / {:d} = {:.3f}",
                _statistics.acceptedMoves, _statistics.attemptedMoves,
                _statistics.moveAcceptanceRatio());
  logger.trace("Accepted over attempted uphill moves: {:d} / {:d} = {:.3f}",
                _statistics.uphillAcceptedMoves,
                _statistics.uphillAttemptedMoves,
                _statistics.uphillAcceptanceRatio());
  logger.trace("Improving move ratio: {:.3f}",
                _statistics.improvingMoveRatio());
  logger.trace("Lowest cost this round: {:d}",
                _statistics.bestCostOfThisRound);
  logger.trace("Lowest cost previous round: {:d}",
                _statistics.bestCostOfPreviousRound);
  logger.trace("Temperature: {:.3f}", _statistics.temperature);
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

    ++_statistics.acceptedMoves;
    return true;
  }
  ++_statistics.uphillAttemptedMoves;

  if (std::exp(static_cast<double>(-delta) / _schedule->temperature()) >=
      _random.floatInRange(0.0f, 1.0f)) {
    ++_statistics.uphillAcceptedMoves;
    ++_statistics.acceptedMoves;
    return true;
  }

  return false;
}
Int Annealer::evaluate(const Cost& cost) const {
  return cost.evaluate(_violationWeight, _objectiveWeight);
}

void Annealer::start() {
  _schedule->start(INITIAL_TEMPERATURE);
  _violationWeight = 1;
  _objectiveWeight = 0;
}

}  // namespace atlantis::search
