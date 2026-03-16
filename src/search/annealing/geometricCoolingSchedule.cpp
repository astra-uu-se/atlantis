#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"

#include "atlantis/search/annealing/types.hpp"

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

void GeometricCoolingSchedule::nextRound(
    const std::shared_ptr<RoundStatistics>& statistics) {
  _temperature *= _coolingRate;

  if (statistics->roundImprovedOnPrevious()) {
    _successiveFutileRounds = 0;
  } else {
    _successiveFutileRounds++;
  }
}

double GeometricCoolingSchedule::temperature() const { return _temperature; }

bool GeometricCoolingSchedule::frozen() const {
  return _successiveFutileRounds >= _successiveFutileRoundsThreshold;
}

std::unique_ptr<AnnealingSchedule> GeometricCoolingSchedule::clone() const {
  return std::make_unique<GeometricCoolingSchedule>(_coolingRate, _successiveFutileRoundsThreshold);
}
}  // namespace atlantis::search
