#include "search/annealing/geometricHeatingSchedule.hpp"

namespace atlantis::search {

GeometricHeatingSchedule::GeometricHeatingSchedule(
    double heatingRate, double minimumUphillAcceptanceRatio)
    : _heatingRate(heatingRate),
      _minimumUphillAcceptanceRatio(minimumUphillAcceptanceRatio) {
  assert(heatingRate >= 1.0);
  assert(minimumUphillAcceptanceRatio > 0);
}

void GeometricHeatingSchedule::start(double initialTemperature) {
  assert(initialTemperature != 0.0);

  _temperature = initialTemperature;
  _lastUphillAcceptanceRatio = 0.0;
}

void GeometricHeatingSchedule::nextRound(const RoundStatistics& statistics) {
  _temperature *= _heatingRate;
  _lastUphillAcceptanceRatio = statistics.uphillAcceptanceRatio();
}

double GeometricHeatingSchedule::temperature() { return _temperature; }

bool GeometricHeatingSchedule::frozen() {
  return _lastUphillAcceptanceRatio >= _minimumUphillAcceptanceRatio;
}

}  // namespace atlantis::search