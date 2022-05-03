#pragma once

#include <memory>
#include <vector>

#include "annealingSchedule.hpp"
#include "geometricCoolingSchedule.hpp"
#include "geometricHeatingSchedule.hpp"
#include "scheduleSequence.hpp"

namespace search {

class AnnealerFacade {
 public:
  template <typename... Schedules>
  static std::unique_ptr<AnnealingSchedule> sequence(Schedules&&... schedules) {
    std::vector<std::unique_ptr<AnnealingSchedule>> vec;
    vec.reserve(sizeof...(schedules));
    (vec.push_back(std::forward<std::unique_ptr<AnnealingSchedule>>(schedules)), ...);
    return sequence(std::move(vec));
  }

  static std::unique_ptr<AnnealingSchedule> sequence(
      ScheduleSequence::ScheduleList schedules) {
    return std::make_unique<ScheduleSequence>(std::move(schedules));
  }

  static std::unique_ptr<AnnealingSchedule> heating(
      double heatingRate, double minimumUphillAcceptanceRatio,
      UInt numberOfMonteCarloSimulations) {
    return std::make_unique<GeometricHeatingSchedule>(
        heatingRate, minimumUphillAcceptanceRatio,
        numberOfMonteCarloSimulations);
  }

  static std::unique_ptr<AnnealingSchedule> cooling(
      double coolingRate, double minimumTemperature,
      UInt numberOfMonteCarloSimulations) {
    return std::make_unique<GeometricCoolingSchedule>(
        coolingRate, minimumTemperature, numberOfMonteCarloSimulations);
  }
};

}  // namespace search
