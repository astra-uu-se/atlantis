#pragma once

#include "annealingSchedule.hpp"

namespace search {

class GeometricCoolingSchedule : public AnnealingSchedule {
 private:
  double _coolingRate;
  double _minimumTemperature;
  UInt _numberOfMonteCarloSimulations;

  double _temperature{0.0};

 public:
  GeometricCoolingSchedule(double coolingRate,
                           double minimumTemperature,
                           UInt numberOfMonteCarloSimulations);

  ~GeometricCoolingSchedule() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  UInt numberOfMonteCarloSimulations() override;
  bool frozen() override;
};

}  // namespace search
