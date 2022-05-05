#include <gtest/gtest.h>

#include "search/annealing/annealerFacade.hpp"

using namespace search;

class ScheduleLoopTest : public testing::Test {
 protected:
  double initialTemp = 5.0;

  double coolingRate = 0.5;
  UInt numberOfMonteCarloSimulations = 10;
  double minimumTemperature = 2.6;

  UInt maximumConsecutiveFutileIterations = 2;

  std::unique_ptr<AnnealingSchedule> schedule;

  void SetUp() override {
    schedule = AnnealerFacade::loop(
        AnnealerFacade::cooling(coolingRate, minimumTemperature,
                                numberOfMonteCarloSimulations),
        maximumConsecutiveFutileIterations);
    schedule->start(initialTemp);
  }
};

TEST_F(ScheduleLoopTest, nested_schedule_is_active) {
  EXPECT_EQ(schedule->temperature(), initialTemp);
  EXPECT_EQ(schedule->numberOfMonteCarloSimulations(),
            numberOfMonteCarloSimulations);

  EXPECT_FALSE(schedule->frozen());
}

TEST_F(ScheduleLoopTest,
       first_freeze_restarts_the_schedule_with_the_old_temperature) {
  schedule->nextRound({});
  EXPECT_FALSE(schedule->frozen());

  // With the call to next round, the inner schedule becomes frozen. So it must
  // be restarted with the temperature before nextRound is called.
  EXPECT_EQ(schedule->temperature(), initialTemp);
}

TEST_F(ScheduleLoopTest, frozen_if_consecutive_rounds_do_not_improve) {
  schedule->nextRound({});
  schedule->nextRound({});

  EXPECT_TRUE(schedule->frozen());
}

TEST_F(ScheduleLoopTest, not_frozen_if_futile_rounds_are_broken_up_by_improving_rounds) {
  schedule->nextRound({});

  RoundStatistics improvingRoundStats{};
  improvingRoundStats.bestCostOfPreviousRound = 10;
  improvingRoundStats.bestCostOfThisRound = 5;
  schedule->nextRound(improvingRoundStats);
  EXPECT_FALSE(schedule->frozen());
  schedule->nextRound({});
  EXPECT_FALSE(schedule->frozen());
  schedule->nextRound({});
  EXPECT_TRUE(schedule->frozen());
}
