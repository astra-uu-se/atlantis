#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "search/annealing/annealerContainer.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::Return;

class DummyAnnealingSchedule : public AnnealingSchedule {
 public:
  ~DummyAnnealingSchedule() override = default;

  MOCK_METHOD(void, start, (double initialTemperature), (override));
  MOCK_METHOD(void, nextRound, (const RoundStatistics& initialTemperature),
              (override));
  MOCK_METHOD(double, temperature, (), (override));
  MOCK_METHOD(bool, frozen, (), (override));
};

class ScheduleLoopTest : public ::testing::Test {
 protected:
  UInt maximumConsecutiveFutileIterations = 2;

  std::unique_ptr<AnnealingSchedule> schedule;
  DummyAnnealingSchedule* inner;

  void SetUp() override {
    auto innerSchedule = std::make_unique<DummyAnnealingSchedule>();
    inner = innerSchedule.get();

    schedule = AnnealerContainer::loop(std::move(innerSchedule),
                                       maximumConsecutiveFutileIterations);
    schedule->start(5.0);
  }
};

TEST_F(ScheduleLoopTest, nested_schedule_is_active) {
  auto temperature = 1.0;

  EXPECT_CALL(*inner, temperature()).WillOnce(Return(temperature));

  EXPECT_EQ(schedule->temperature(), temperature);
  EXPECT_FALSE(schedule->frozen());
}

TEST_F(ScheduleLoopTest,
       first_freeze_restarts_the_schedule_with_the_old_temperature) {
  auto restartTemp = 10.0;
  EXPECT_CALL(*inner, frozen()).WillOnce(Return(true));
  EXPECT_CALL(*inner, temperature()).WillRepeatedly(Return(restartTemp));

  schedule->nextRound({});
  EXPECT_FALSE(schedule->frozen());
  EXPECT_EQ(schedule->temperature(), restartTemp);
}

TEST_F(ScheduleLoopTest, frozen_if_consecutive_rounds_do_not_improve) {
  EXPECT_CALL(*inner, frozen()).WillOnce(Return(true)).WillOnce(Return(true));

  schedule->nextRound({});
  schedule->nextRound({});

  EXPECT_TRUE(schedule->frozen());
}

TEST_F(ScheduleLoopTest,
       not_frozen_if_futile_rounds_are_broken_up_by_improving_rounds) {
  EXPECT_CALL(*inner, frozen())
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));

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

}  // namespace atlantis::testing