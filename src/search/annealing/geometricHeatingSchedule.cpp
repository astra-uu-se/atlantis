#include "search/annealing/geometricHeatingSchedule.hpp"

search::GeometricHeatingSchedule::GeometricHeatingSchedule(
    double heatingRate, double minimumUphillAcceptanceRatio,
    UInt numberOfMonteCarloSimulations)
    : _heatingRate(heatingRate),
      _minimumUphillAcceptanceRatio(minimumUphillAcceptanceRatio),
      _numberOfMonteCarloSimulations(numberOfMonteCarloSimulations) {
  assert(heatingRate >= 1.0);
  assert(minimumUphillAcceptanceRatio > 0);
}

void search::GeometricHeatingSchedule::start(double initialTemperature) {
  assert(initialTemperature != 0.0);

  _temperature = initialTemperature;
  _lastUphillAcceptanceRatio = 0.0;
}

void search::GeometricHeatingSchedule::nextRound(
    const search::RoundStatistics& statistics) {
  _temperature *= _heatingRate;
  _lastUphillAcceptanceRatio = statistics.uphillAcceptanceRatio();
}

double search::GeometricHeatingSchedule::temperature() { return _temperature; }

UInt search::GeometricHeatingSchedule::numberOfMonteCarloSimulations() {
  return _numberOfMonteCarloSimulations;
}

bool search::GeometricHeatingSchedule::frozen() {
  return _lastUphillAcceptanceRatio >= _minimumUphillAcceptanceRatio;
}
