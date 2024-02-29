#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"

namespace atlantis::search {

GeometricCoolingSchedule::GeometricCoolingSchedule(
    double coolingRate, UInt successiveFutileRoundsThreshold)
    : _coolingRate(coolingRate),
      _successiveFutileRoundsThreshold(successiveFutileRoundsThreshold) {
  assert(coolingRate < 1.0);
  assert(successiveFutileRoundsThreshold > 0);
}

void GeometricCoolingSchedule::start(double initialTemperature) {
  assert(initialTemperature != 0.0);
  _temperature = initialTemperature;
  _successiveFutileRounds = 0;
}

void GeometricCoolingSchedule::nextRound(const RoundStatistics& statistics) {
  _temperature *= _coolingRate;

  if (statistics.roundImprovedOnPrevious()) {
    _successiveFutileRounds = 0;
  } else {
    _successiveFutileRounds++;
  }
}

double GeometricCoolingSchedule::temperature() { return _temperature; }

bool GeometricCoolingSchedule::frozen() {
  return _successiveFutileRounds >= _successiveFutileRoundsThreshold;
}

}  // namespace atlantis::search
