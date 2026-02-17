#pragma once

#include <vector>

#include "annealerContainer.hpp"
#include "annealingSchedule.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {



class AnnealingScheduleContainerFactory {
public:
  virtual ~AnnealingScheduleContainerFactory() = default;

  virtual std::unique_ptr<AnnealingSchedule> create() = 0;
};



class HeatingScheduleFactory : public AnnealingScheduleContainerFactory {
  double _heatingRate{};
  double _minimumUphillAcceptanceRatio{};

public:
  explicit HeatingScheduleFactory(const double heatingRate,
                                 const double minimumUphillAcceptanceRatio):
    _heatingRate(heatingRate),
    _minimumUphillAcceptanceRatio(minimumUphillAcceptanceRatio) {}

  std::unique_ptr<AnnealingSchedule> create() override {
    return AnnealerContainer::heating(_heatingRate, _minimumUphillAcceptanceRatio);
  }
};



class CoolingScheduleFactory : public AnnealingScheduleContainerFactory {
  double _coolingRate;
  UInt _successiveFutileRoundsThreshold;

public:
  explicit CoolingScheduleFactory(const double coolingRate,
                                 const UInt successiveFutileRoundsThreshold):
  _coolingRate(coolingRate), _successiveFutileRoundsThreshold(successiveFutileRoundsThreshold) {}

  std::unique_ptr<AnnealingSchedule> create() override {
    return AnnealerContainer::cooling(_coolingRate, _successiveFutileRoundsThreshold);
  }
};



class SequenceFactory : public AnnealingScheduleContainerFactory {
  std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> _schedule;

public:
  explicit SequenceFactory(std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> schedule):
  _schedule(std::move(schedule)) {}

  std::unique_ptr<AnnealingSchedule> create() override {
    std::vector<std::unique_ptr<AnnealingSchedule>> schedules;
    for (const auto& schedule : _schedule)
      schedules.emplace_back(schedule->create());

    return AnnealerContainer::sequence(std::move(schedules));
  }
};



class LoopScheduleFactory : public AnnealingScheduleContainerFactory {
  std::unique_ptr<AnnealingScheduleContainerFactory> _schedule;
  UInt _maximumConsecutiveFutileRounds;

public:
  explicit LoopScheduleFactory(
     std::unique_ptr<AnnealingScheduleContainerFactory> schedule, UInt maximumConsecutiveFutileRounds):
  _schedule(std::move(schedule)), _maximumConsecutiveFutileRounds(maximumConsecutiveFutileRounds) {}

  std::unique_ptr<AnnealingSchedule> create() override {
    return AnnealerContainer::loop(_schedule->create(), _maximumConsecutiveFutileRounds);
  }
};

}  // namespace atlantis::search