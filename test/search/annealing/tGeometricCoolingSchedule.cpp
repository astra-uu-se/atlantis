#include <gtest/gtest.h>

#include "search/annealing/annealerFacade.hpp"
#include "search/annealing/geometricCoolingSchedule.hpp"

using namespace search;

class GeometricCoolingScheduleTest : public testing::Test {
 protected:
  double cooling = 0.5;
  double initialTemp = 5.0;
  double acceptedMoveRatio = 0.001;
  UInt numberOfMonteCarloSimulations = 10;

  std::unique_ptr<AnnealingSchedule> schedule;

  void SetUp() override {
    schedule = AnnealerFacade::cooling(cooling, acceptedMoveRatio, numberOfMonteCarloSimulations);
    schedule->start(initialTemp);
  }
};

TEST_F(GeometricCoolingScheduleTest, schedule_is_initialised) {
  EXPECT_EQ(schedule->temperature(), initialTemp);
  EXPECT_EQ(schedule->numberOfMonteCarloSimulations(), numberOfMonteCarloSimulations);
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

TEST_F(GeometricCoolingScheduleTest, frozen_when_temperature_crosses_minimum_threshold) {
  EXPECT_FALSE(schedule->frozen());

  RoundStatistics stats{};
  stats.attemptedMoves = 5000;
  stats.acceptedMoves = 100;
  schedule->nextRound(stats);
  EXPECT_FALSE(schedule->frozen());

  stats.acceptedMoves = 1;
  schedule->nextRound(stats);
  EXPECT_TRUE(schedule->frozen());
}

TEST_F(GeometricCoolingScheduleTest, restarting_frozen_schedule_is_unfrozen) {
  EXPECT_FALSE(schedule->frozen());

  RoundStatistics stats{};
  stats.attemptedMoves = 5000;
  stats.acceptedMoves = 1;
  schedule->nextRound(stats);
  EXPECT_TRUE(schedule->frozen());

  schedule->start(initialTemp);
  EXPECT_FALSE(schedule->frozen());
}
