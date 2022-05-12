#include "search/annealing/annealerFacade.hpp"

#include "search/annealing/geometricCoolingSchedule.hpp"
#include "search/annealing/geometricHeatingSchedule.hpp"
#include "search/annealing/scheduleLoop.hpp"
#include "search/annealing/scheduleSequence.hpp"

using namespace search;

std::unique_ptr<AnnealingSchedule> AnnealerFacade::sequence(
    std::vector<std::unique_ptr<AnnealingSchedule>> schedules) {
  return std::make_unique<ScheduleSequence>(std::move(schedules));
}

std::unique_ptr<AnnealingSchedule> AnnealerFacade::heating(
    double heatingRate, double minimumUphillAcceptanceRatio) {
  return std::make_unique<GeometricHeatingSchedule>(
      heatingRate, minimumUphillAcceptanceRatio);
}

std::unique_ptr<AnnealingSchedule> AnnealerFacade::cooling(
    double coolingRate, double moveAcceptanceRatio) {
  return std::make_unique<GeometricCoolingSchedule>(coolingRate,
                                                    moveAcceptanceRatio);
}

std::unique_ptr<AnnealingSchedule> AnnealerFacade::loop(
    std::unique_ptr<AnnealingSchedule> schedule, UInt numberOfIterations) {
  return std::make_unique<ScheduleLoop>(std::move(schedule),
                                        numberOfIterations);
}
