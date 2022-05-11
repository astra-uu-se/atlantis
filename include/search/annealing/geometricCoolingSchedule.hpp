#pragma once

#include "annealingSchedule.hpp"

namespace search {

class GeometricCoolingSchedule : public AnnealingSchedule {
 private:
  double _coolingRate;
  double _minimumMoveAcceptanceRatio;
  UInt _numberOfMonteCarloSimulations;

  double _temperature{0.0};
  double _lastRoundMoveAcceptanceRatio{-1.0};

 public:
  GeometricCoolingSchedule(double coolingRate,
                           double moveAcceptanceRatio,
                           UInt numberOfMonteCarloSimulations);

  ~GeometricCoolingSchedule() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  UInt numberOfMonteCarloSimulations() override;
  bool frozen() override;
};

}  // namespace search
