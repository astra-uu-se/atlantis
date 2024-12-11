#include "atlantis/search/searchProcedure.hpp"

#include <chrono>
#include <string>

#include "atlantis/logging/logger.hpp"

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

SearchStatistics SearchProcedure::run(SearchController& controller,
                                      Annealer& annealer,
                                      logging::Logger& logger) {
  auto rounds = std::make_unique<CounterStatistic>("Rounds");
  auto initialisations = std::make_unique<CounterStatistic>("Initialisations");
  auto moves = std::make_unique<CounterStatistic>("Moves");

  do {
    initialisations->increment();

    logger.timedProcedure(logging::Level::LVL_TRACE, "initialize assignment",
                          [&] { _assignment.initialize(_random); });

    if (_assignment.satisfiesConstraints()) {
      controller.onSolution(_assignment);
      _objective.tighten();
    }

    annealer.start();

    while (controller.shouldRun(_assignment) && !annealer.isFinished()) {
      logger.timedProcedure(logging::Level::LVL_TRACE, "round", [&] {
        while (controller.shouldRun(_assignment) &&
               annealer.runMonteCarloSimulation()) {
          const auto cost = _assignment.performProbe(_random);

          if (annealer.acceptMove(cost)) {
            _assignment.commitLastProbe();
            moves->increment();
            if (_assignment.satisfiesConstraints()) {
              controller.onSolution(_assignment);
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

  std::vector<std::unique_ptr<Statistic>> statistics;
  statistics.push_back(std::move(rounds));
  statistics.push_back(std::move(initialisations));
  statistics.push_back(std::move(moves));

  controller.onFinish();

  return SearchStatistics{std::move(statistics)};
}

}  // namespace atlantis::search
