#include "search/annealing/geometricCoolingSchedule.hpp"

search::GeometricCoolingSchedule::GeometricCoolingSchedule(
    double coolingRate, double minimumTemperature,
    UInt numberOfMonteCarloSimulations)
    : _coolingRate(coolingRate),
      _minimumTemperature(minimumTemperature),
      _numberOfMonteCarloSimulations(numberOfMonteCarloSimulations) {
  assert(coolingRate < 1.0);
  assert(minimumTemperature > 0);
}

void search::GeometricCoolingSchedule::start(double initialTemperature) {
  assert(initialTemperature != 0.0);

  _temperature = initialTemperature;
}

void search::GeometricCoolingSchedule::nextRound(
    const search::RoundStatistics&) {
  _temperature *= _coolingRate;
}

double search::GeometricCoolingSchedule::temperature() { return _temperature; }

UInt search::GeometricCoolingSchedule::numberOfMonteCarloSimulations() {
  return _numberOfMonteCarloSimulations;
}

bool search::GeometricCoolingSchedule::frozen() {
  return temperature() < _minimumTemperature;
}
