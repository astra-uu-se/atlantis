#include "search/searchProcedure.hpp"

#include <chrono>
#include <string>

static void logRoundStatistics(logging::Logger& logger,
                               const search::RoundStatistics& statistics) {
  logger.trace("Accepted over attempted moves: {:d} / {:d} = {:.3f}",
               statistics.acceptedMoves, statistics.attemptedMoves,
               statistics.moveAcceptanceRatio());
  logger.trace("Accepted over attempted uphill moves: {:d} / {:d} = {:.3f}",
               statistics.uphillAcceptedMoves, statistics.uphillAttemptedMoves,
               statistics.uphillAcceptanceRatio());
  logger.trace("Lowest cost this round: {:d}", statistics.bestCostOfThisRound);
  logger.trace("Lowest cost previous round: {:d}",
               statistics.bestCostOfPreviousRound);
}

search::SearchStatistics search::SearchProcedure::run(
    SearchController& controller, search::Annealer& annealer,
    logging::Logger& logger) {
  auto rounds = std::make_unique<CounterStatistic>("Rounds");
  auto initialisations = std::make_unique<CounterStatistic>("Initialisations");
  auto moves = std::make_unique<CounterStatistic>("Moves");

  auto startTime = std::chrono::steady_clock::now();

  do {
    initialisations->increment();

    logger.timed<void>(logging::Level::DEBUG, "initialise assignment", [&] {
      _assignment.assign([&](auto& modifications) {
        _neighbourhood.initialise(_random, modifications);
      });
    });

    while (controller.shouldRun(_assignment) && !annealer.isFinished()) {
      logger.timed<void>(logging::Level::TRACE, "round", [&] {
        while (controller.shouldRun(_assignment) &&
               annealer.runMonteCarloSimulation()) {
          bool madeMove =
              _neighbourhood.randomMove(_random, _assignment, annealer);

          if (madeMove) {
            moves->increment();
          }

          if (madeMove && _assignment.satisfiesConstraints()) {
            controller.onSolution(_assignment);
            _objective.tighten();
          }
        }

        logger.indented<void>(logging::Level::TRACE, "Round statistics", [&] {
          logRoundStatistics(logger, annealer.currentRoundStatistics());
        });
        annealer.nextRound();
        rounds->increment();
      });
    }
  } while (controller.shouldRun(_assignment));

  auto duration = std::chrono::steady_clock::now() - startTime;

  std::vector<std::unique_ptr<Statistic>> statistics;
  statistics.push_back(std::move(rounds));
  statistics.push_back(std::move(initialisations));
  statistics.push_back(std::move(moves));

  controller.onFinish();

  return SearchStatistics{std::move(statistics)};
}
