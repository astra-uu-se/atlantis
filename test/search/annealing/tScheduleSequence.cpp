#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/annealing/scheduleSequence.hpp"
#include "atlantis/search/annealing/types.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::Return;

class DummyAnnealingSchedule : public AnnealingSchedule {
 public:
  MOCK_METHOD(void, start, (double initialTemperature), (override));
  MOCK_METHOD(void, nextRound, (const RoundStatistics& initialTemperature),
              (override));
  MOCK_METHOD(double, temperature, (), (override));
  MOCK_METHOD(bool, frozen, (), (override));
};

class ScheduleSequenceTest : public ::testing::Test {
 protected:
  std::shared_ptr<AnnealingSchedule> schedule;

  ScheduleSequence& sequence() {
    EXPECT_NE(schedule, nullptr);
    return dynamic_cast<ScheduleSequence&>(*schedule);
  }

  DummyAnnealingSchedule& inner(size_t index) {
    EXPECT_LE(index, sequence().size());
    return dynamic_cast<DummyAnnealingSchedule&>(sequence().at(index));
  }

  void SetUp() override {
    auto seq = std::vector<std::unique_ptr<AnnealingSchedule>>();
    seq.emplace_back(std::make_unique<DummyAnnealingSchedule>());
    seq.emplace_back(std::make_unique<DummyAnnealingSchedule>());

    schedule = AnnealerContainer::sequence(std::move(seq));
    schedule->start(1.0);
  }
};

TEST_F(ScheduleSequenceTest, first_schedule_is_active) {
  EXPECT_CALL(inner(0), temperature()).WillOnce(Return(1.0));

  EXPECT_EQ(schedule->temperature(), 1.0);
  EXPECT_FALSE(schedule->frozen());
}

TEST_F(ScheduleSequenceTest, second_schedule_is_active_after_first_freezes) {
  EXPECT_CALL(inner(0), frozen()).WillOnce(Return(true));

  EXPECT_CALL(inner(1), frozen()).WillRepeatedly(Return(false));
  EXPECT_CALL(inner(1), temperature()).WillRepeatedly(Return(2.0));

  schedule->nextRound(RoundStatistics(1.0));
  EXPECT_FALSE(schedule->frozen());
  schedule->nextRound(RoundStatistics(1.0));
  EXPECT_FALSE(schedule->frozen());

  EXPECT_EQ(schedule->temperature(), 2.0);
}

TEST_F(ScheduleSequenceTest, frozen_if_sequence_is_finished) {
  EXPECT_CALL(inner(0), frozen()).WillRepeatedly(Return(true));
  EXPECT_CALL(inner(1), frozen()).WillRepeatedly(Return(true));

  schedule->nextRound(RoundStatistics(1.0));
  schedule->nextRound(RoundStatistics(1.0));
  EXPECT_TRUE(schedule->frozen());
}

}  // namespace atlantis::testing
