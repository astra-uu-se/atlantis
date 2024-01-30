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
  double initialTemperature{0.5};
};

TEST_F(ScheduleLoopTest, nested_schedule_is_active) {
  auto temperature = 1.0;

  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, temperature()).WillOnce(Return(temperature));

  auto loopSchedule = AnnealerContainer::loop(std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  EXPECT_EQ(loopSchedule->temperature(), temperature);
  EXPECT_FALSE(loopSchedule->frozen());
}

TEST_F(ScheduleLoopTest,
       first_freeze_restarts_the_schedule_with_the_old_temperature) {
  auto restartTemp = 10.0;

  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, frozen()).WillOnce(Return(true));
  EXPECT_CALL(*dummySchedule, temperature()).WillRepeatedly(Return(restartTemp));

  auto loopSchedule = AnnealerContainer::loop(std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  loopSchedule->nextRound({});
  EXPECT_FALSE(loopSchedule->frozen());
  EXPECT_EQ(loopSchedule->temperature(), restartTemp);
}

TEST_F(ScheduleLoopTest, frozen_if_consecutive_rounds_do_not_improve) {
  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, frozen()).WillOnce(Return(true)).WillOnce(Return(true));

  auto loopSchedule = AnnealerContainer::loop(std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  loopSchedule->nextRound({});
  loopSchedule->nextRound({});

  EXPECT_TRUE(loopSchedule->frozen());
}

TEST_F(ScheduleLoopTest,
       not_frozen_if_futile_rounds_are_broken_up_by_improving_rounds) {
  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, frozen())
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));

  auto loopSchedule = AnnealerContainer::loop(std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  loopSchedule->nextRound({});

  RoundStatistics improvingRoundStats{};
  improvingRoundStats.bestCostOfPreviousRound = 10;
  improvingRoundStats.bestCostOfThisRound = 5;
  loopSchedule->nextRound(improvingRoundStats);
  EXPECT_FALSE(loopSchedule->frozen());
  loopSchedule->nextRound({});
  EXPECT_FALSE(loopSchedule->frozen());
  loopSchedule->nextRound({});
  EXPECT_TRUE(loopSchedule->frozen());
}

}  // namespace atlantis::testing