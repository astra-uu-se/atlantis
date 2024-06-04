#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"

namespace atlantis::search {

class GeometricCoolingSchedule : public AnnealingSchedule {
 private:
  double _coolingRate;
  UInt _successiveFutileRoundsThreshold;

  double _temperature{0.0};
  UInt _successiveFutileRounds{0};

 public:
  GeometricCoolingSchedule(double coolingRate,
                           UInt successiveFutileRoundsThreshold);

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  bool frozen() override;
};

}  // namespace atlantis::search
