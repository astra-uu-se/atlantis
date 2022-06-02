#include <gtest/gtest.h>

#include "search/annealing/annealerFacade.hpp"
#include "search/annealing/geometricCoolingSchedule.hpp"

using namespace search;

class GeometricCoolingScheduleTest : public testing::Test {
 protected:
  double cooling = 0.5;
  double initialTemp = 5.0;
  UInt successiveFutileRoundsThreshold = 2;

  std::unique_ptr<AnnealingSchedule> schedule;

  void SetUp() override {
    schedule = AnnealerFacade::cooling(cooling, successiveFutileRoundsThreshold);
    schedule->start(initialTemp);
  }
};

TEST_F(GeometricCoolingScheduleTest, schedule_is_initialised) {
  EXPECT_EQ(schedule->temperature(), initialTemp);
}

TEST_F(GeometricCoolingScheduleTest, start_resets_temperature) {
  schedule->start(2.0);
  EXPECT_EQ(schedule->temperature(), 2.0);
}

TEST_F(GeometricCoolingScheduleTest, temperature_decreases_geometrically) {
  schedule->start(initialTemp);
  EXPECT_EQ(schedule->temperature(), initialTemp);

  RoundStatistics stats{};
  stats.attemptedMoves = 100;
  stats.acceptedMoves = 10;
  schedule->nextRound(stats);
  EXPECT_EQ(schedule->temperature(), initialTemp * cooling);

  schedule->nextRound(stats);
  EXPECT_EQ(schedule->temperature(), initialTemp * cooling * cooling);
}

TEST_F(GeometricCoolingScheduleTest, frozen_if_rounds_no_longer_improve) {
  EXPECT_FALSE(schedule->frozen());

  RoundStatistics stats{};
  stats.bestCostOfThisRound = 5;
  stats.bestCostOfPreviousRound = 5;
  schedule->nextRound(stats);
  EXPECT_FALSE(schedule->frozen());

  schedule->nextRound(stats);
  EXPECT_TRUE(schedule->frozen());
}

TEST_F(GeometricCoolingScheduleTest, restarting_frozen_schedule_is_unfrozen) {
  EXPECT_FALSE(schedule->frozen());

  RoundStatistics stats{};
  stats.bestCostOfThisRound = 5;
  stats.bestCostOfPreviousRound = 5;
  schedule->nextRound(stats);
  schedule->nextRound(stats);
  EXPECT_TRUE(schedule->frozen());

  schedule->start(initialTemp);
  EXPECT_FALSE(schedule->frozen());
}
