#pragma once

#include <memory>
#include <vector>

#include "atlantis/search/annealing/annealingSchedule.hpp"

namespace atlantis::search {

class AnnealerContainer {
 public:
  template <typename... Schedules>
  static std::unique_ptr<AnnealingSchedule> sequence(Schedules&&... schedules) {
    std::vector<std::unique_ptr<AnnealingSchedule>> vec;
    vec.reserve(sizeof...(schedules));
    (vec.push_back(std::forward<std::unique_ptr<AnnealingSchedule>>(schedules)),
     ...);
    return sequence(std::move(vec));
  }

  static std::unique_ptr<AnnealingSchedule> sequence(
      std::vector<std::unique_ptr<AnnealingSchedule>> schedules);

  static std::unique_ptr<AnnealingSchedule> loop(
      std::unique_ptr<AnnealingSchedule> schedule, UInt numberOfIterations);

  static std::unique_ptr<AnnealingSchedule> heating(
      double heatingRate, double minimumUphillAcceptanceRatio);

  static std::unique_ptr<AnnealingSchedule> cooling(
      double coolingRate, UInt successiveFutileRoundsThreshold);
};

}  // namespace atlantis::search
