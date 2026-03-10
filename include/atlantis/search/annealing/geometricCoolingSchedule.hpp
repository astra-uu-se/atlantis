#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

class GeometricCoolingSchedule : public AnnealingSchedule {
  double _coolingRate;
  UInt _successiveFutileRoundsThreshold;

  double _temperature{0.0};
  UInt _successiveFutileRounds{0};

 public:
  GeometricCoolingSchedule(double coolingRate,
                           UInt successiveFutileRoundsThreshold);

  void start(double initialTemperature) override;
  void nextRound(const std::shared_ptr<RoundStatistics>& statistics) override;
  [[nodiscard]] double temperature() const override;
  [[nodiscard]] bool frozen() const override;
    [[nodiscard]] std::unique_ptr<AnnealingSchedule> clone() const override;

  [[nodiscard]] double coolingRate() const { return _coolingRate; }
  [[nodiscard]] UInt successiveFutileRoundsThreshold() const { return _successiveFutileRoundsThreshold; }

};

}  // namespace atlantis::search
