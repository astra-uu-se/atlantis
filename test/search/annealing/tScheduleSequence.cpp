#include <gtest/gtest.h>

#include "search/annealing/annealerFacade.hpp"
#include "search/annealing/scheduleSequence.hpp"

using namespace search;

class ScheduleSequenceTest : public testing::Test {
 protected:
  double initialTemp = 5.0;

  double heatingRate = 2.0;
  UInt heatingMonteCarlo = 10;
  double uphillAcceptanceRate = 0.5;

  double coolingRate = 0.25;
  double minimumTemperature = 1.26;
  UInt coolingMonteCarlo = 5;

  std::unique_ptr<AnnealingSchedule> schedule;

  void SetUp() override {
    schedule = AnnealerFacade::sequence(
        AnnealerFacade::heating(heatingRate, uphillAcceptanceRate,
                                heatingMonteCarlo),
        AnnealerFacade::cooling(coolingRate, minimumTemperature,
                                coolingMonteCarlo));
    schedule->start(initialTemp);
  }
};

TEST_F(ScheduleSequenceTest, first_schedule_is_active) {
  EXPECT_EQ(schedule->temperature(), initialTemp);
  EXPECT_EQ(schedule->numberOfMonteCarloSimulations(), heatingMonteCarlo);

  EXPECT_FALSE(schedule->frozen());
}

TEST_F(ScheduleSequenceTest, second_schedule_is_active_after_first_freezes) {
  RoundStatistics stats{};
  stats.uphillAcceptedMoves = 4;
  stats.uphillAttemptedMoves = 10;
  schedule->nextRound(stats);
  EXPECT_FALSE(schedule->frozen());
  EXPECT_EQ(schedule->numberOfMonteCarloSimulations(), heatingMonteCarlo);

  stats.uphillAcceptedMoves = 6;
  schedule->nextRound(stats);
  EXPECT_FALSE(schedule->frozen());

  EXPECT_EQ(schedule->numberOfMonteCarloSimulations(), coolingMonteCarlo);
  EXPECT_EQ(schedule->temperature(), initialTemp * heatingRate);
}

TEST_F(ScheduleSequenceTest, frozen_if_sequence_is_finished) {
  RoundStatistics stats{};
  stats.uphillAcceptedMoves = 6;
  stats.uphillAttemptedMoves = 10;
  schedule->nextRound(stats);

  RoundStatistics stats2{};
  schedule->nextRound(stats2);

  EXPECT_TRUE(schedule->frozen());
}
