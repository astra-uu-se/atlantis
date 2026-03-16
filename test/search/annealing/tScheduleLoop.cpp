#include <boost/asio/execution/schedule.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/search/annealing/scheduleLoop.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "testHelper.hpp"


namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::A;
using ::testing::An;
using ::testing::Return;
using ::testing::ContainerEq;

class ScheduleLoopTest : public ::testing::Test {
 protected:
  UInt maximumConsecutiveFutileIterations = 2;
  double initialTemperature{0.5};
    [[nodiscard]] static const DummyAnnealingSchedule& inner(const ScheduleLoop& schedule) {
        auto const* ptr = dynamic_cast<DummyAnnealingSchedule const*>(&(schedule.inner()));
        EXPECT_NE(ptr, nullptr);
        return *ptr;
    }
};

TEST_F(ScheduleLoopTest, nested_schedule_is_active) {
  const auto temperature = 1.0;

  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, temperature()).WillOnce(Return(temperature));

  ScheduleLoop loopSchedule(
      std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule.start(initialTemperature);

  EXPECT_EQ(loopSchedule.temperature(), temperature);
  EXPECT_FALSE(loopSchedule.frozen());
  EXPECT_THAT(inner(loopSchedule).temperatures, ContainerEq(std::vector<double>{initialTemperature}));
  EXPECT_TRUE(inner(loopSchedule).roundStatistics.empty());
}

TEST_F(ScheduleLoopTest,
       first_freeze_restarts_the_schedule_with_the_old_temperature) {
  const auto restartTemp = 10.0;

  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();
  EXPECT_CALL(*dummySchedule, frozen()).WillOnce(Return(true));
  EXPECT_CALL(*dummySchedule, temperature())
        .WillRepeatedly(Return(restartTemp));

  ScheduleLoop loopSchedule(
      std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule.start(initialTemperature);

  loopSchedule.nextRound(std::make_shared<RoundStatistics>());
  EXPECT_FALSE(loopSchedule.frozen());
  EXPECT_EQ(loopSchedule.temperature(), restartTemp);
  EXPECT_THAT(inner(loopSchedule).temperatures, ContainerEq(std::vector<double>{initialTemperature, restartTemp}));
  EXPECT_EQ(inner(loopSchedule).roundStatistics.size(), 1);
}

TEST_F(ScheduleLoopTest, frozen_if_consecutive_rounds_do_not_improve) {
  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, frozen())
      .WillOnce(Return(true))
      .WillOnce(Return(true));
  EXPECT_CALL(*dummySchedule, temperature())
      .WillRepeatedly(Return(initialTemperature));

  ScheduleLoop loopSchedule(
      std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule.start(initialTemperature);

  loopSchedule.nextRound(std::make_shared<RoundStatistics>());
  loopSchedule.nextRound(std::make_shared<RoundStatistics>());

  EXPECT_TRUE(loopSchedule.frozen());
  EXPECT_THAT(inner(loopSchedule).temperatures, ContainerEq(std::vector<double>{initialTemperature, initialTemperature, initialTemperature}));
  EXPECT_EQ(inner(loopSchedule).roundStatistics.size(), 2);
}

TEST_F(ScheduleLoopTest,
       not_frozen_if_futile_rounds_are_broken_up_by_improving_rounds) {
  ScheduleLoop loopSchedule(std::make_unique<DummyAnnealingSchedule>(),
                              maximumConsecutiveFutileIterations);

  const auto& dummySchedule = dynamic_cast<const DummyAnnealingSchedule&>(
      loopSchedule.inner());

  EXPECT_CALL(dummySchedule, frozen())
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(dummySchedule, temperature())
      .WillRepeatedly(Return(initialTemperature));

  loopSchedule.start(initialTemperature);

  loopSchedule.nextRound(std::make_shared<RoundStatistics>());

  const auto improvingRoundStats = std::make_shared<RoundStatistics>();
  improvingRoundStats->bestCostOfPreviousRound = Cost(10);
  improvingRoundStats->bestCostOfThisRound = Cost(5);
  loopSchedule.nextRound(improvingRoundStats);
  EXPECT_FALSE(loopSchedule.frozen());
  loopSchedule.nextRound(std::make_shared<RoundStatistics>());
  EXPECT_FALSE(loopSchedule.frozen());
  loopSchedule.nextRound(std::make_shared<RoundStatistics>());
  EXPECT_TRUE(loopSchedule.frozen());
  EXPECT_THAT(inner(loopSchedule).temperatures, ContainerEq(std::vector<double>{initialTemperature, initialTemperature, initialTemperature, initialTemperature}));
  EXPECT_EQ(inner(loopSchedule).roundStatistics.size(), 4);
}

}  // namespace atlantis::testing
