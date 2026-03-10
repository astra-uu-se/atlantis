#pragma once

#include <memory>
#include <vector>

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {

std::unique_ptr<AnnealingSchedule> annealingScheduleSequence(
  std::vector<std::unique_ptr<AnnealingSchedule>>&& schedules);

std::unique_ptr<AnnealingSchedule> annealingScheduleLoop(
  std::unique_ptr<AnnealingSchedule>&& schedule, UInt numberOfIterations);

std::unique_ptr<AnnealingSchedule> annealingScheduleHeating(
  double heatingRate, double minimumUphillAcceptanceRatio);

std::unique_ptr<AnnealingSchedule> annealingScheduleCooling(
  double coolingRate, UInt successiveFutileRoundsThreshold);

}  // namespace atlantis::search
