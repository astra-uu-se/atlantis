#pragma once

#include "annealingSchedule.hpp"

namespace search {

class GeometricHeatingSchedule : public AnnealingSchedule {
 private:
  double _heatingRate;
  double _minimumUphillAcceptanceRatio;
  UInt _numberOfMonteCarloSimulations;

  double _temperature{0.0};
  double _lastUphillAcceptanceRatio{0.0};

 public:
  GeometricHeatingSchedule(double heatingRate,
                           double minimumUphillAcceptanceRatio,
                           UInt numberOfMonteCarloSimulations);

  ~GeometricHeatingSchedule() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  UInt numberOfMonteCarloSimulations() override;
  bool frozen() override;
};

}  // namespace search
