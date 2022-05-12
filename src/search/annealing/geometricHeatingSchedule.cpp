#include "search/annealing/geometricHeatingSchedule.hpp"

search::GeometricHeatingSchedule::GeometricHeatingSchedule(
    double heatingRate, double minimumUphillAcceptanceRatio)
    : _heatingRate(heatingRate),
      _minimumUphillAcceptanceRatio(minimumUphillAcceptanceRatio) {
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

bool search::GeometricHeatingSchedule::frozen() {
  return _lastUphillAcceptanceRatio >= _minimumUphillAcceptanceRatio;
}
