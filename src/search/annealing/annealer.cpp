#include "atlantis/search/annealing/annealer.hpp"

#include <cmath>
#include <limits>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::search {

static UInt reqMovesPerRound(const size_t numSearchVars) {
  return static_cast<UInt>(static_cast<double>(128 * numSearchVars) /
                           std::log2(numSearchVars));
}

Annealer::Annealer(RandomProvider& random,
                   std::unique_ptr<AnnealingSchedule>&& schedule,
                   const Assignment& assignment)
    : _random(random),
      _schedule(std::move(schedule)),
      _cost(bool{assignment.hasViolation()}, assignment.objectiveDirection()),
      _statistics(std::make_shared<RoundStatistics>(INITIAL_TEMPERATURE)),
      _requiredMovesPerRound(reqMovesPerRound(assignment.searchVars().size())) {
}

bool Annealer::isFinished() const { return _schedule->frozen(); }

void Annealer::nextRound() {
  _schedule->nextRound(_statistics);
  _statistics->nextRound(_schedule->temperature());
  _attemptedMovesPerRound = 0;
}

bool Annealer::shouldRunRound() const {
  return _attemptedMovesPerRound < _requiredMovesPerRound;
}

bool Annealer::acceptMove(const Cost& cost) {
  ++_attemptedMovesPerRound;

  const bool ret = accept(cost);

  if (!shouldRunRound()) {
    nextRound();
  }

  if (ret) {
    _cost = cost;
  }

  return ret;
}

void Annealer::logRoundStatistics(logging::Logger& logger) {
  logger.trace("Accepted over attempted moves: {:d} / {:d} = {:.3f}",
               _statistics->acceptedMoves, _statistics->attemptedMoves,
               _statistics->moveAcceptanceRatio());
  logger.trace("Accepted over attempted uphill moves: {:d} / {:d} = {:.3f}",
               _statistics->uphillAcceptedMoves,
               _statistics->uphillAttemptedMoves,
               _statistics->uphillAcceptanceRatio());
  logger.trace("Improving move ratio: {:.3f}",
               _statistics->improvingMoveRatio());
  logger.trace("Lowest cost this round: {:s}",
               _statistics->bestCostOfThisRound.toString());
  logger.trace("Lowest cost previous round: {:s}",
               _statistics->bestCostOfPreviousRound.toString());
  logger.trace("Temperature: {:.3f}", _statistics->temperature);
}

bool Annealer::accept(const Cost& move) {
  _statistics->attemptedMoves++;

  if (move <= _cost) {
    if (move < _cost) {
      _statistics->improvingMoves++;
    }

    if (move < _statistics->bestCostOfThisRound) {
      _statistics->bestCostOfThisRound = move;
    }

    ++_statistics->acceptedMoves;

    return true;
  }
  ++_statistics->uphillAttemptedMoves;

  Int delta;
  if (overflow::subOverflow(evaluate(move), evaluate(_cost), &delta)) {
    return false;
  }

  if (std::exp(static_cast<double>(-delta) / _schedule->temperature()) >=
      _random.floatInRange(0.0f, 1.0f)) {
    ++_statistics->uphillAcceptedMoves;
    ++_statistics->acceptedMoves;
    return true;
  }

  return false;
}
Int Annealer::evaluate(const Cost& cost) const {
  return cost.evaluate(_violationWeight, _objectiveWeight);
}

void Annealer::start() {
  _schedule->start(INITIAL_TEMPERATURE);
  _violationWeight = _cost.hasViolation() ? 1 : 0;
  _objectiveWeight = _cost.hasViolation() ? 0 : 1;
}

}  // namespace atlantis::search
