#pragma once

#include "atlantis/search/annealing/annealingSchedule.hpp"

namespace atlantis::search {

class GeometricHeatingSchedule : public AnnealingSchedule {
  double _heatingRate;
  double _minimumUphillAcceptanceRatio;

  double _temperature{0.0};
  double _lastUphillAcceptanceRatio{0.0};

 public:
  GeometricHeatingSchedule(double heatingRate,
                           double minimumUphillAcceptanceRatio);

  void start(double initialTemperature) override;
  void nextRound(const std::shared_ptr<RoundStatistics>& statistics) override;
  [[nodiscard]] double temperature() const override;
  [[nodiscard]] bool frozen() const override;
    [[nodiscard]] std::unique_ptr<AnnealingSchedule> clone() const override;
  [[nodiscard]] double heatingRate() const { return _heatingRate; }
  [[nodiscard]] double minimumUphillAcceptanceRatio() const { return _minimumUphillAcceptanceRatio; }
};

}  // namespace atlantis::search
