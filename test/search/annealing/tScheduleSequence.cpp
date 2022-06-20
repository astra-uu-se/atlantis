#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "search/annealing/annealerContainer.hpp"

using namespace search;

class DummyAnnealingSchedule : public AnnealingSchedule {
 public:
  ~DummyAnnealingSchedule() override = default;

  MOCK_METHOD(void, start, (double initialTemperature), (override));
  MOCK_METHOD(void, nextRound, (const RoundStatistics& initialTemperature),
              (override));
  MOCK_METHOD(double, temperature, (), (override));
  MOCK_METHOD(bool, frozen, (), (override));
};

class ScheduleSequenceTest : public testing::Test {
 protected:
  DummyAnnealingSchedule* inner1;
  DummyAnnealingSchedule* inner2;
  std::unique_ptr<AnnealingSchedule> schedule;

  void SetUp() override {
    auto innerSchedule1 = std::make_unique<DummyAnnealingSchedule>();
    inner1 = innerSchedule1.get();

    auto innerSchedule2 = std::make_unique<DummyAnnealingSchedule>();
    inner2 = innerSchedule2.get();

    // I don't know how to pass in innerSchedule1 directly, since then the
    // sequence method doesn't compile. It expects
    // std::unique_ptr<AnnealingSchedule> but is given
    // std::unique_ptr<DummyAnnealingSchedule>.
    std::unique_ptr<AnnealingSchedule> i1 = std::move(innerSchedule1);
    std::unique_ptr<AnnealingSchedule> i2 = std::move(innerSchedule2);

    schedule = AnnealerContainer::sequence(i1, i2);
    schedule->start(1.0);
  }
};

TEST_F(ScheduleSequenceTest, first_schedule_is_active) {
  EXPECT_CALL(*inner1, temperature()).WillOnce(testing::Return(1.0));

  EXPECT_EQ(schedule->temperature(), 1.0);
  EXPECT_FALSE(schedule->frozen());
}

TEST_F(ScheduleSequenceTest, second_schedule_is_active_after_first_freezes) {
  EXPECT_CALL(*inner1, frozen()).WillOnce(testing::Return(true));

  EXPECT_CALL(*inner2, frozen()).WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*inner2, temperature()).WillRepeatedly(testing::Return(2.0));

  schedule->nextRound({});
  EXPECT_FALSE(schedule->frozen());
  schedule->nextRound({});
  EXPECT_FALSE(schedule->frozen());

  EXPECT_EQ(schedule->temperature(), 2.0);
}

TEST_F(ScheduleSequenceTest, frozen_if_sequence_is_finished) {
  EXPECT_CALL(*inner1, frozen()).WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*inner2, frozen()).WillRepeatedly(testing::Return(true));

  schedule->nextRound({});
  schedule->nextRound({});
  EXPECT_TRUE(schedule->frozen());
}
