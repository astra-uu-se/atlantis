#include "atlantis/search/searchProcedure.hpp"

#include <chrono>

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

int SearchProcedure::run(SearchController& controller, Annealer& annealer,
                         logging::Logger& logger) {
  auto rounds = std::make_unique<CounterStatistic>("Rounds");
  auto initialisations = std::make_unique<CounterStatistic>("Initialisations");
  auto moves = std::make_unique<CounterStatistic>("Moves");

  do {
    initialisations->increment();

    logger.timedProcedure(logging::Level::LVL_TRACE, "initialize assignment",
                          [&] { _assignment.initialize(_random); });

    if (_assignment.satisfiesConstraints()) {
      auto q = initialisations->clone();
      auto r = moves->clone();
      _solution = controller.onSolution(_assignment);
      // _solution = controller.onSolution(
      //     _assignment, makeStats(*rounds, *initialisations, *moves));
      _objective.tighten();
    }

    annealer.start();

    while (controller.shouldRun(_assignment) && !annealer.isFinished()) {
      logger.timedProcedure(logging::Level::LVL_TRACE, "round", [&] {
        while (controller.shouldRun(_assignment) && annealer.shouldRunRound()) {
          const auto cost = _assignment.performProbe(_random);

          if (annealer.acceptMove(cost)) {
            _assignment.commitLastProbe();
            moves->increment();
            if (_assignment.satisfiesConstraints()) {
              _solution = controller.onSolution(_assignment);
              // _solution = controller.onSolution(
              //     _assignment, makeStats(*rounds, *initialisations, *moves));
              _objective.tighten();
            }
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
  } while (controller.shouldRun(_assignment));

  controller.onFinish();

  return 1;
}

}  // namespace atlantis::search
