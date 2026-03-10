#pragma once

#include <vector>

#include "atlantis/search/annealing/annealingSchedule.hpp"
#include "atlantis/types.hpp"

namespace atlantis::search {



class AnnealingScheduleContainerFactory {
public:
  virtual ~AnnealingScheduleContainerFactory() = default;

  virtual std::unique_ptr<AnnealingSchedule> create() = 0;
};



class HeatingScheduleFactory : public AnnealingScheduleContainerFactory {
  double _heatingRate;
  double _minimumUphillAcceptanceRatio;

public:
  explicit HeatingScheduleFactory(double heatingRate,
                                 double minimumUphillAcceptanceRatio);
};



class CoolingScheduleFactory : public AnnealingScheduleContainerFactory {
  double _coolingRate;
  UInt _successiveFutileRoundsThreshold;

public:
  explicit CoolingScheduleFactory(double coolingRate,
                                 UInt successiveFutileRoundsThreshold);

  std::unique_ptr<AnnealingSchedule> create() override;
};



class SequenceFactory : public AnnealingScheduleContainerFactory {
  std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> _schedule;

public:
  explicit SequenceFactory(std::vector<std::unique_ptr<AnnealingScheduleContainerFactory>> schedule):
  _schedule(std::move(schedule)) {}

  std::unique_ptr<AnnealingSchedule> create() override;
};



class LoopScheduleFactory : public AnnealingScheduleContainerFactory {
  std::unique_ptr<AnnealingScheduleContainerFactory> _schedule;
  UInt _maximumConsecutiveFutileRounds;

public:
  explicit LoopScheduleFactory(
     std::unique_ptr<AnnealingScheduleContainerFactory> schedule, UInt maximumConsecutiveFutileRounds);

  std::unique_ptr<AnnealingSchedule> create() override;
};

}  // namespace atlantis::search