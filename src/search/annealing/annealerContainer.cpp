#include "atlantis/search/annealing/annealerContainer.hpp"

#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"
#include "atlantis/search/annealing/geometricHeatingSchedule.hpp"
#include "atlantis/search/annealing/scheduleLoop.hpp"
#include "atlantis/search/annealing/scheduleSequence.hpp"

namespace atlantis::search {

std::shared_ptr<AnnealingSchedule> AnnealerContainer::sequence(
    std::vector<std::shared_ptr<AnnealingSchedule>> schedules) {
  return std::make_shared<ScheduleSequence>(std::move(schedules));
}

std::shared_ptr<AnnealingSchedule> AnnealerContainer::heating(
    double heatingRate, double minimumUphillAcceptanceRatio) {
  return std::make_shared<GeometricHeatingSchedule>(
      heatingRate, minimumUphillAcceptanceRatio);
}

std::shared_ptr<AnnealingSchedule> AnnealerContainer::cooling(
    double coolingRate, UInt successiveFutileRoundsThreshold) {
  return std::make_shared<GeometricCoolingSchedule>(
      coolingRate, successiveFutileRoundsThreshold);
}

std::shared_ptr<AnnealingSchedule> AnnealerContainer::loop(
    std::shared_ptr<AnnealingSchedule> schedule, UInt numberOfIterations) {
  return std::make_shared<ScheduleLoop>(std::move(schedule),
                                        numberOfIterations);
}

}  // namespace atlantis::search
