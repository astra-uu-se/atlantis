#include "search/annealing/geometricCoolingSchedule.hpp"

search::GeometricCoolingSchedule::GeometricCoolingSchedule(
    double coolingRate, double moveAcceptanceRatio)
    : _coolingRate(coolingRate),
      _minimumMoveAcceptanceRatio(moveAcceptanceRatio) {
  assert(coolingRate < 1.0);
  assert(moveAcceptanceRatio > 0);
}

void search::GeometricCoolingSchedule::start(double initialTemperature) {
  assert(initialTemperature != 0.0);
  _lastRoundMoveAcceptanceRatio = -1.0;
  _temperature = initialTemperature;
}

void search::GeometricCoolingSchedule::nextRound(
    const search::RoundStatistics& statistics) {
  _temperature *= _coolingRate;
  _lastRoundMoveAcceptanceRatio = statistics.improvingMoveRatio();
}

double search::GeometricCoolingSchedule::temperature() { return _temperature; }

bool search::GeometricCoolingSchedule::frozen() {
  return _lastRoundMoveAcceptanceRatio >= 0 &&
         _lastRoundMoveAcceptanceRatio <= _minimumMoveAcceptanceRatio;
}
