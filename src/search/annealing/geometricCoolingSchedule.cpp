#include "search/annealing/geometricCoolingSchedule.hpp"

search::GeometricCoolingSchedule::GeometricCoolingSchedule(
    double coolingRate, UInt successiveFutileRoundsThreshold)
    : _coolingRate(coolingRate),
      _successiveFutileRoundsThreshold(successiveFutileRoundsThreshold) {
  assert(coolingRate < 1.0);
  assert(successiveFutileRoundsThreshold > 0);
}

void search::GeometricCoolingSchedule::start(double initialTemperature) {
  assert(initialTemperature != 0.0);
  _temperature = initialTemperature;
  _successiveFutileRounds = 0;
}

void search::GeometricCoolingSchedule::nextRound(
    const search::RoundStatistics& statistics) {
  _temperature *= _coolingRate;

  if (statistics.roundImprovedOnPrevious()) {
    _successiveFutileRounds = 0;
  } else {
    _successiveFutileRounds++;
  }
}

double search::GeometricCoolingSchedule::temperature() { return _temperature; }

bool search::GeometricCoolingSchedule::frozen() {
  return _successiveFutileRounds >= _successiveFutileRoundsThreshold;
}
