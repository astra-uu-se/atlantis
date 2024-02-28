#include "atlantis/search/annealing/annealerContainer.hpp"

namespace atlantis::search {

std::unique_ptr<AnnealingSchedule> AnnealerContainer::sequence(
    std::vector<std::unique_ptr<AnnealingSchedule>> schedules) {
  return std::make_unique<ScheduleSequence>(std::move(schedules));
}

std::unique_ptr<AnnealingSchedule> AnnealerContainer::heating(
    double heatingRate, double minimumUphillAcceptanceRatio) {
  return std::make_unique<GeometricHeatingSchedule>(
      heatingRate, minimumUphillAcceptanceRatio);
}

std::unique_ptr<AnnealingSchedule> AnnealerContainer::cooling(
    double coolingRate, UInt successiveFutileRoundsThreshold) {
  return std::make_unique<GeometricCoolingSchedule>(
      coolingRate, successiveFutileRoundsThreshold);
}

std::unique_ptr<AnnealingSchedule> AnnealerContainer::loop(
    std::unique_ptr<AnnealingSchedule> schedule, UInt numberOfIterations) {
  return std::make_unique<ScheduleLoop>(std::move(schedule),
                                        numberOfIterations);
}

}  // namespace atlantis::search
