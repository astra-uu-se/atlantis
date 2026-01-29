#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/annealing/scheduleLoop.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::A;
using ::testing::An;
using ::testing::Return;

class DummyAnnealingSchedule : public AnnealingSchedule {
 public:
  MOCK_METHOD(void, start, (double initialTemperature), (override));
  MOCK_METHOD(void, nextRound,
              (const std::shared_ptr<RoundStatistics>& initialTemperature),
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
  const auto temperature = 1.0;

  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, temperature()).WillOnce(Return(temperature));
  EXPECT_CALL(*dummySchedule, start(initialTemperature)).WillOnce(Return());

  const auto loopSchedule = AnnealerContainer::loop(
      std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  EXPECT_EQ(loopSchedule->temperature(), temperature);
  EXPECT_FALSE(loopSchedule->frozen());
}

TEST_F(ScheduleLoopTest,
       first_freeze_restarts_the_schedule_with_the_old_temperature) {
  const auto restartTemp = 10.0;

  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, frozen()).WillOnce(Return(true));
  EXPECT_CALL(*dummySchedule, temperature())
      .WillRepeatedly(Return(restartTemp));
  EXPECT_CALL(*dummySchedule, start(A<double>())).WillRepeatedly(Return());
  EXPECT_CALL(*dummySchedule,
              nextRound(A<const std::shared_ptr<RoundStatistics>&>()))
      .WillOnce(Return());

  const auto loopSchedule = AnnealerContainer::loop(
      std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  loopSchedule->nextRound(std::make_shared<RoundStatistics>());
  EXPECT_FALSE(loopSchedule->frozen());
  EXPECT_EQ(loopSchedule->temperature(), restartTemp);
}

TEST_F(ScheduleLoopTest, frozen_if_consecutive_rounds_do_not_improve) {
  auto dummySchedule = std::make_unique<DummyAnnealingSchedule>();

  EXPECT_CALL(*dummySchedule, frozen())
      .WillOnce(Return(true))
      .WillOnce(Return(true));
  EXPECT_CALL(*dummySchedule, start(A<double>())).WillRepeatedly(Return());
  EXPECT_CALL(*dummySchedule,
              nextRound(A<const std::shared_ptr<RoundStatistics>&>()))
      .WillRepeatedly(Return());
  EXPECT_CALL(*dummySchedule, temperature())
      .WillRepeatedly(Return(initialTemperature));

  const auto loopSchedule = AnnealerContainer::loop(
      std::move(dummySchedule), maximumConsecutiveFutileIterations);
  loopSchedule->start(initialTemperature);

  loopSchedule->nextRound(std::make_shared<RoundStatistics>());
  loopSchedule->nextRound(std::make_shared<RoundStatistics>());

  EXPECT_TRUE(loopSchedule->frozen());
}

TEST_F(ScheduleLoopTest,
       not_frozen_if_futile_rounds_are_broken_up_by_improving_rounds) {
  const auto loopSchedule =
      AnnealerContainer::loop(std::make_unique<DummyAnnealingSchedule>(),
                              maximumConsecutiveFutileIterations);

  auto& dummySchedule = dynamic_cast<DummyAnnealingSchedule&>(
      dynamic_cast<search::ScheduleLoop&>(*loopSchedule).inner());

  EXPECT_CALL(dummySchedule, frozen())
      .WillOnce(Return(false))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(dummySchedule, start(A<double>())).WillRepeatedly(Return());
  EXPECT_CALL(dummySchedule, temperature())
      .WillRepeatedly(Return(initialTemperature));
  EXPECT_CALL(dummySchedule,
              nextRound(A<const std::shared_ptr<RoundStatistics>&>()))
      .WillRepeatedly(Return());

  loopSchedule->start(initialTemperature);

  loopSchedule->nextRound(std::make_shared<RoundStatistics>());

  const auto improvingRoundStats = std::make_shared<RoundStatistics>();
  improvingRoundStats->bestCostOfPreviousRound = 10;
  improvingRoundStats->bestCostOfThisRound = 5;
  loopSchedule->nextRound(improvingRoundStats);
  EXPECT_FALSE(loopSchedule->frozen());
  loopSchedule->nextRound(std::make_shared<RoundStatistics>());
  EXPECT_FALSE(loopSchedule->frozen());
  loopSchedule->nextRound(std::make_shared<RoundStatistics>());
  EXPECT_TRUE(loopSchedule->frozen());
}

}  // namespace atlantis::testing
