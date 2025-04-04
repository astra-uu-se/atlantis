#pragma once

#include <memory>
#include <vector>

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

class AnnealerContainer {
 public:
  static std::shared_ptr<AnnealingSchedule> sequence(
      std::vector<std::shared_ptr<AnnealingSchedule>> schedules);

  static std::shared_ptr<AnnealingSchedule> loop(
      std::shared_ptr<AnnealingSchedule> schedule, UInt numberOfIterations);

  static std::shared_ptr<AnnealingSchedule> heating(
      double heatingRate, double minimumUphillAcceptanceRatio);

  static std::shared_ptr<AnnealingSchedule> cooling(
      double coolingRate, UInt successiveFutileRoundsThreshold);
};

}  // namespace atlantis::search
