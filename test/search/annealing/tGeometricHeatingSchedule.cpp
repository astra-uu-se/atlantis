#include <gtest/gtest.h>

#include "search/annealing/annealerFacade.hpp"
#include "search/annealing/geometricHeatingSchedule.hpp"

using namespace search;

class GeometricHeatingScheduleTest : public testing::Test {
 protected:
  double heatingRate = 1.2;
  double initialTemp = 0.1;
  double minimumUphillMoveAcceptanceRatio = 0.5;
  UInt numberOfMonteCarloSimulations = 10;

  std::unique_ptr<AnnealingSchedule> schedule;

  void SetUp() override {
    schedule = AnnealerFacade::heating(heatingRate, minimumUphillMoveAcceptanceRatio, numberOfMonteCarloSimulations);
    schedule->start(initialTemp);
  }
};

TEST_F(GeometricHeatingScheduleTest, schedule_is_initialised) {
  EXPECT_EQ(schedule->temperature(), initialTemp);
  EXPECT_EQ(schedule->numberOfMonteCarloSimulations(), numberOfMonteCarloSimulations);
}

TEST_F(GeometricHeatingScheduleTest, start_resets_temperature) {
  schedule->start(2.0);
  EXPECT_EQ(schedule->temperature(), 2.0);
}

TEST_F(GeometricHeatingScheduleTest, temperature_increases_geometrically) {
  schedule->start(initialTemp);
  EXPECT_EQ(schedule->temperature(), initialTemp);

  schedule->nextRound({});
  EXPECT_EQ(schedule->temperature(), initialTemp * heatingRate);

  schedule->nextRound({});
  EXPECT_EQ(schedule->temperature(), initialTemp * heatingRate * heatingRate);
}

TEST_F(GeometricHeatingScheduleTest, frozen_when_accepted_uphill_moves_surpasses_threshold) {
  EXPECT_FALSE(schedule->frozen());

  RoundStatistics stats{};
  stats.uphillAcceptedMoves = 10;
  stats.uphillAttemptedMoves = 21;
  schedule->nextRound(stats);
  EXPECT_FALSE(schedule->frozen());

  stats.uphillAttemptedMoves = 19;
  schedule->nextRound(stats);
  EXPECT_TRUE(schedule->frozen());
}

TEST_F(GeometricHeatingScheduleTest, restarting_frozen_schedule_is_unfrozen) {
  EXPECT_FALSE(schedule->frozen());

  RoundStatistics stats{};
  stats.uphillAcceptedMoves = 10;
  stats.uphillAttemptedMoves = 19;
  schedule->nextRound(stats);
  EXPECT_TRUE(schedule->frozen());

  schedule->start(initialTemp);
  EXPECT_FALSE(schedule->frozen());
}
