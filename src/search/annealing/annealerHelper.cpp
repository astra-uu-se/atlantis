#include "./annealerHelper.hpp"

#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"
#include "atlantis/search/annealing/geometricHeatingSchedule.hpp"
#include "atlantis/search/annealing/scheduleLoop.hpp"
#include "atlantis/search/annealing/scheduleSequence.hpp"

namespace atlantis::search {

std::unique_ptr<AnnealingSchedule> annealingScheduleSequence(
    std::vector<std::unique_ptr<AnnealingSchedule>>&& schedules) {
  return std::make_unique<ScheduleSequence>(std::move(schedules));
}

std::unique_ptr<AnnealingSchedule> annealingScheduleHeating(
    double heatingRate, double minimumUphillAcceptanceRatio) {
  return std::make_unique<GeometricHeatingSchedule>(
      heatingRate, minimumUphillAcceptanceRatio);
}

std::unique_ptr<AnnealingSchedule> annealingScheduleCooling(
    double coolingRate, UInt successiveFutileRoundsThreshold) {
  return std::make_unique<GeometricCoolingSchedule>(
      coolingRate, successiveFutileRoundsThreshold);
}

std::unique_ptr<AnnealingSchedule> annealingScheduleLoop(
    std::unique_ptr<AnnealingSchedule>&& schedule, UInt numberOfIterations) {
  return std::make_unique<ScheduleLoop>(std::move(schedule),
                                        numberOfIterations);
}

}  // namespace atlantis::search
