#include "search/searchProcedure.hpp"

#include <chrono>
#include <sstream>
#include <string>

static std::string formatDuration(std::chrono::nanoseconds duration) {
  auto seconds = duration.count() / 1'000'000'000;
  auto milliseconds = duration.count() / 1'000'000;
  auto microseconds = duration.count() / 1'000;
  auto nanoseconds = duration.count();

  std::stringstream output;

  if (seconds > 0) {
    output << seconds << "s";
  } else if (milliseconds > 0) {
    output << milliseconds << "ms";
  } else if (microseconds > 0) {
    output << microseconds << "μs";
  } else {
    output << nanoseconds << "ns";
  }

  return output.str();
}

search::SearchStatistics search::SearchProcedure::run(
    SearchController& controller, search::Annealer& annealer) {
  auto exploredLocals = std::make_unique<CounterStatistic>("Locals");
  auto initialisations = std::make_unique<CounterStatistic>("Initialisations");
  auto moves = std::make_unique<CounterStatistic>("Moves");
  auto solutions = std::make_unique<CounterStatistic>("Solutions");

  auto startTime = std::chrono::steady_clock::now();

  do {
    initialisations->increment();
    _assignment.assign([&](auto& modifications) {
      _neighbourhood.initialise(_random, modifications);
    });

    while (controller.shouldRun(_assignment) && annealer.isFinished()) {
      while (controller.shouldRun(_assignment) &&
             annealer.runMonteCarloSimulation()) {
        bool madeMove =
            _neighbourhood.randomMove(_random, _assignment, annealer);

        if (madeMove) {
          logDebug(
              "Committed a move. New cost: " << _assignment.cost().toString());
          moves->increment();
        }

        if (madeMove && _assignment.satisfiesConstraints()) {
          controller.onSolution(_assignment);
          _objective.tighten();
          solutions->increment();
        }
      }

      annealer.nextRound();
      exploredLocals->increment();
    }
  } while (controller.shouldRun(_assignment));

  auto duration = std::chrono::steady_clock::now() - startTime;

  std::vector<std::unique_ptr<Statistic>> statistics;
  statistics.push_back(std::move(exploredLocals));
  statistics.push_back(std::move(initialisations));
  statistics.push_back(std::move(moves));
  statistics.push_back(std::move(solutions));
  statistics.push_back(std::make_unique<ValueStatistic<std::string>>(
      "Search time", formatDuration(duration)));

  controller.onFinish();

  return SearchStatistics{std::move(statistics)};
}
