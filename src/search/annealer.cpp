#include "search/annealer.hpp"

bool search::Annealer::isFinished() const { return _schedule.frozen(); }

void search::Annealer::nextRound() {
  _schedule.nextRound(_statistics);
  _statistics = {};
  _localIterations = 0;
}

bool search::Annealer::runMonteCarloSimulation() {
  ++_localIterations;
  return _localIterations < _schedule.numberOfMonteCarloSimulations();
}

bool search::Annealer::accept(Int delta) const {
  return std::exp(static_cast<double>(-delta) / _schedule.temperature()) >=
         _random.floatInRange(0.0f, 1.0f);
}
