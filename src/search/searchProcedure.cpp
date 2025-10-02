#include "atlantis/search/searchProcedure.hpp"

#include <chrono>
#include <iostream>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/annealer.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/savedAssignment.hpp"
#include "atlantis/search/searchController.hpp"

namespace atlantis::search {

static void logRoundStatistics(logging::Logger& logger,
                               const RoundStatistics& statistics) {
  logger.trace("Accepted over attempted moves: {:d} / {:d} = {:.3f}",
               statistics.acceptedMoves, statistics.attemptedMoves,
               statistics.moveAcceptanceRatio());
  logger.trace("Accepted over attempted uphill moves: {:d} / {:d} = {:.3f}",
               statistics.uphillAcceptedMoves, statistics.uphillAttemptedMoves,
               statistics.uphillAcceptanceRatio());
  logger.trace("Improving move ratio: {:.3f}", statistics.improvingMoveRatio());
  logger.trace("Lowest cost this round: {:d}", statistics.bestCostOfThisRound);
  logger.trace("Lowest cost previous round: {:d}",
               statistics.bestCostOfPreviousRound);
  logger.trace("Temperature: {:.3f}", statistics.temperature);
}

// TODO: either use or remove this function
SearchStatistics makeStats(const Statistic& rounds,
                           const Statistic& initialisations,
                           const Statistic& moves) {
  std::vector<std::unique_ptr<Statistic>> statistics;
  statistics.push_back(rounds.clone());
  statistics.push_back(initialisations.clone());
  statistics.push_back(moves.clone());
  SearchStatistics stats = SearchStatistics(std::move(statistics));
  return stats;
}

SavedAssignment SearchProcedure::saveAssignment() const {
  return SavedAssignment(_assignment, _outputVarIds);
}

void SearchProcedure::tightenSearch() {
  if (_searchType == SearchType::PARALLEL) {
    _objective.tighten();
  } else {
    _objective.tighten(_savedAssignment->getCost());
    if (_searchType == SearchType::BEAMSEARCH)
      _assignment.setAssignment(_savedAssignment.value());
  }
}

// TODO: Add communication to non-satisfying cases
void SearchProcedure::onImprovement(SearchController& searchController) {
  // Prevent over-communication before an initial 0-violation solution
  // has been found
  if (!_hasSolution && _savedAssignment.has_value() &&
      _savedAssignment->getCost().isBetterThan(_assignment.getCost())) {
    return;
  }

  _savedAssignment = saveAssignment();

  bool isBest =
      _threadController->trySolution(_threadId, _savedAssignment.value());
  if (!isBest) _savedAssignment = _threadController->getSolution();

  if (!_hasSolution && _savedAssignment->getCost().getViolation() == 0)
    _hasSolution = true;

  // onSolution is only called if the new solution is better than ALL previous
  // solutions
  if (isBest && _assignment.satisfiesConstraints())
    searchController.onSolution(_savedAssignment.value());

  tightenSearch();
}

int SearchProcedure::run(SearchController& searchController, Annealer& annealer,
                         logging::Logger& logger) {
  auto rounds = std::make_unique<CounterStatistic>("Rounds");
  auto initialisations = std::make_unique<CounterStatistic>("Initialisations");
  auto moves = std::make_unique<CounterStatistic>("Moves");

  do {
    initialisations->increment();

    logger.timedProcedure(logging::Level::LVL_TRACE, "initialize assignment",
                          [&] { _assignment.initialize(_random); });

    // TODO: handle this case: this should call some separate version
    if (_assignment.satisfiesConstraints()) onImprovement(searchController);

    annealer.start();

    while (searchController.shouldRun(_assignment) && !annealer.isFinished()) {
      logger.timedProcedure(logging::Level::LVL_TRACE, "round", [&] {
        while (searchController.shouldRun(_assignment) &&
               annealer.shouldRunRound()) {
          const auto cost = _assignment.performProbe(_random);

          if (annealer.acceptMove(cost)) {
            _assignment.commitLastProbe();
            moves->increment();
            if (!_hasSolution || _assignment.satisfiesConstraints())
              onImprovement(searchController);
          }
        }

        logger.indentedProcedure(
            logging::Level::LVL_TRACE, "Round statistics", [&] {
              logRoundStatistics(logger, annealer.currentRoundStatistics());
            });
        annealer.nextRound();
        rounds->increment();
      });
    }
  } while (searchController.shouldRun(_assignment));

  searchController.onFinish();

  return 1;
}

}  // namespace atlantis::search
