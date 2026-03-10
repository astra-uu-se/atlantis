#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/search/annealing/scheduleSequence.hpp"
#include "atlantis/search/annealing/types.hpp"
#include "testHelper.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::A;
using ::testing::An;
using ::testing::Return;
using ::testing::ContainerEq;

class ScheduleSequenceTest : public ::testing::Test {
 protected:
  std::unique_ptr<ScheduleSequence> schedule;


  [[nodiscard]] const DummyAnnealingSchedule& inner(const size_t index) const {
    EXPECT_LE(index, schedule->size());
    return dynamic_cast<const DummyAnnealingSchedule&>(schedule->at(index));
  }

  void SetUp() override {
    auto seq = std::vector<std::unique_ptr<AnnealingSchedule>>();
    seq.emplace_back(std::make_unique<DummyAnnealingSchedule>());
    seq.emplace_back(std::make_unique<DummyAnnealingSchedule>());

    schedule = std::make_unique<ScheduleSequence>(std::move(seq));
  }
};

TEST_F(ScheduleSequenceTest, first_schedule_is_active) {
  EXPECT_CALL(inner(0), temperature()).WillOnce(Return(1.0));
  schedule->start(1.0);
  EXPECT_THAT(inner(0).temperatures, ContainerEq(std::vector<double>{1.0}));
  EXPECT_TRUE(inner(0).roundStatistics.empty());

  EXPECT_EQ(schedule->temperature(), 1.0);
  EXPECT_FALSE(schedule->frozen());
}

TEST_F(ScheduleSequenceTest, second_schedule_is_active_after_first_freezes) {
  EXPECT_CALL(inner(0), temperature()).WillOnce(Return(1.0));
  EXPECT_CALL(inner(0), frozen()).WillOnce(Return(true));

  EXPECT_CALL(inner(1), frozen()).WillRepeatedly(Return(false));
  EXPECT_CALL(inner(1), temperature()).WillRepeatedly(Return(2.0));

  schedule->start(1.0);
  schedule->nextRound(std::make_shared<RoundStatistics>(1.0));
  EXPECT_FALSE(schedule->frozen());
  schedule->nextRound(std::make_shared<RoundStatistics>(1.0));
  EXPECT_FALSE(schedule->frozen());

  EXPECT_EQ(schedule->temperature(), 2.0);

  EXPECT_THAT(inner(0).temperatures, ContainerEq(std::vector<double>{1.0}));
  EXPECT_EQ(inner(0).roundStatistics.size(), 1);

  EXPECT_THAT(inner(1).temperatures, ContainerEq(std::vector<double>{1.0}));
  EXPECT_EQ(inner(0).roundStatistics.size(), 1);

}

TEST_F(ScheduleSequenceTest, frozen_if_sequence_is_finished) {
  EXPECT_CALL(inner(0), frozen()).WillRepeatedly(Return(true));
  EXPECT_CALL(inner(0), temperature()).WillOnce(Return(1.0));

  EXPECT_CALL(inner(1), frozen()).WillRepeatedly(Return(true));
  EXPECT_CALL(inner(1), temperature()).WillOnce(Return(2.0));

  schedule->start(1.0);

  schedule->nextRound(std::make_shared<RoundStatistics>(1.0));
  schedule->nextRound(std::make_shared<RoundStatistics>(1.0));
  EXPECT_TRUE(schedule->frozen());

  EXPECT_THAT(inner(0).temperatures, ContainerEq(std::vector<double>{1.0}));
  EXPECT_EQ(inner(0).roundStatistics.size(), 1);
  EXPECT_THAT(inner(1).temperatures, ContainerEq(std::vector<double>{1.0}));
  EXPECT_EQ(inner(1).roundStatistics.size(), 1);

}

}  // namespace atlantis::testing
