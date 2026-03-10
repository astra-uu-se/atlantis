#include "atlantis/search/annealing/AnnealingScheduleContainerFactory.hpp"

#include <fstream>
#include <utility>

#define JSON_NO_IO
#define JSON_HAS_CPP_17
#include <nlohmann/json.hpp>

#include "atlantis/search/annealing/geometricCoolingSchedule.hpp"

#include "./annealerHelper.hpp"

namespace atlantis::search {
CoolingScheduleFactory::
CoolingScheduleFactory(const double coolingRate, const UInt successiveFutileRoundsThreshold):
    _coolingRate(coolingRate), _successiveFutileRoundsThreshold(successiveFutileRoundsThreshold) {}

std::unique_ptr<AnnealingSchedule> CoolingScheduleFactory::create() {
    return annealingScheduleCooling(_coolingRate, _successiveFutileRoundsThreshold);
}

std::unique_ptr<AnnealingSchedule> SequenceFactory::create() {
    std::vector<std::unique_ptr<AnnealingSchedule>> schedules;
    schedules.reserve(_schedule.size());
    for (const auto& schedule : _schedule) {
        schedules.emplace_back(schedule->create());
    }

    return annealingScheduleSequence(std::move(schedules));
}

LoopScheduleFactory::LoopScheduleFactory(std::unique_ptr<AnnealingScheduleContainerFactory> schedule,
    UInt maximumConsecutiveFutileRounds):
    _schedule(std::move(schedule)), _maximumConsecutiveFutileRounds(maximumConsecutiveFutileRounds) {}

std::unique_ptr<AnnealingSchedule> LoopScheduleFactory::create() {
    return annealingScheduleLoop(_schedule->create(), _maximumConsecutiveFutileRounds);
}
}  // namespace atlantis::search
