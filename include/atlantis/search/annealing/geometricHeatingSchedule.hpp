#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"

namespace atlantis::search {

class GeometricHeatingSchedule : public AnnealingSchedule {
 private:
  double _heatingRate;
  double _minimumUphillAcceptanceRatio;

  double _temperature{0.0};
  double _lastUphillAcceptanceRatio{0.0};

 public:
  GeometricHeatingSchedule(double heatingRate,
                           double minimumUphillAcceptanceRatio);

  ~GeometricHeatingSchedule() override = default;

  void start(double initialTemperature) override;
  void nextRound(const RoundStatistics& statistics) override;
  double temperature() override;
  bool frozen() override;
};

}  // namespace atlantis::search
