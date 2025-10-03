#pragma once

#include <memory>
#include <vector>

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

class AnnealerContainer {
 public:
  static std::unique_ptr<AnnealingSchedule> sequence(
      std::vector<std::unique_ptr<AnnealingSchedule>> &&schedules);

  static std::unique_ptr<AnnealingSchedule> loop(
      std::unique_ptr<AnnealingSchedule>&& schedule, UInt numberOfIterations);

  static std::unique_ptr<AnnealingSchedule> heating(
      double heatingRate, double minimumUphillAcceptanceRatio);

  static std::unique_ptr<AnnealingSchedule> cooling(
      double coolingRate, UInt successiveFutileRoundsThreshold);
};

}  // namespace atlantis::search
