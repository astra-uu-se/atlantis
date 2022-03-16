
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/types.hpp"
#include "variables/committableInt.hpp"

namespace {
class CommittableIntTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(CommittableIntTest, CommittableIntConstructor) {
  std::uniform_int_distribution<> distribution(
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max() - 1);

  // Random timestamp
  Timestamp initTimestamp = std::max(0, distribution(gen));
  // Random inital value
  Int value = distribution(gen);

  CommittableInt committableInt = CommittableInt(initTimestamp, value);

  // Get the current value at the initial timestamp return the initial value
  EXPECT_EQ(committableInt.value(initTimestamp), value);

  // get at the current value at the initial timestamp. Returns the initial
  // value
  EXPECT_EQ(committableInt.value(initTimestamp), value);

  Timestamp otherTimestamp = std::max(0, distribution(gen));
  // Get the current value at another timestamp.
  // Should still return the initial value (as no other value has been
  // committed)
  EXPECT_EQ(committableInt.value(otherTimestamp), value);

  // get the current value at another timestamp. Should still return the
  // initial value (as no other value has been committed)
  EXPECT_EQ(committableInt.value(otherTimestamp), value);
}

TEST_F(CommittableIntTest, CommittableIntSetGetValue) {
  // A uniform distribution for the initial value and timestamp
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  // A uniform distribution for the other (next) timestamp
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int nextValue = distribution2(gen);

  // Create a CommittableInt
  CommittableInt committableInt = CommittableInt(initTimestamp, initValue);

  // Update the value for the other (next) timestamp
  committableInt.setValue(nextTimestamp, nextValue);

  // The value at the initial timestamp is still the initial value
  EXPECT_EQ(committableInt.value(initTimestamp), initValue);
  // The value at the next timestamp is now the next value
  EXPECT_EQ(committableInt.value(nextTimestamp), nextValue);

  Timestamp otherTimestamp = distribution2(gen);
  while (otherTimestamp == nextTimestamp) {
    otherTimestamp = distribution2(gen);
  }
  // The value for any timestamp except nextTimestamp is still the initial value
  EXPECT_EQ(committableInt.value(otherTimestamp), initValue);
}

TEST_F(CommittableIntTest, CommittableIntIncValue) {
  std::uniform_int_distribution<> distribution(
      std::numeric_limits<int>::min() + 10,
      std::numeric_limits<int>::max() - 10);

  Timestamp initTimestamp = std::max(0, distribution(gen));
  Int committedValue = distribution(gen);

  CommittableInt committableInt = CommittableInt(initTimestamp, committedValue);

  Timestamp nextTimestamp;
  Int nextValue;
  // Increase -10, -9, ..., -1, 1, 2, ..., 10
  for (Int increment = -10; increment <= 10; ++increment) {
    if (increment == 0) {
      continue;
    }
    nextTimestamp = initTimestamp + increment;
    nextValue = committedValue + increment;

    // Not the same timestamp as init timestamp, increment will be based on:
    //   saved
    committableInt.incValue(nextTimestamp, increment);

    EXPECT_EQ(committableInt.value(initTimestamp), committedValue);
    EXPECT_EQ(committableInt.value(nextTimestamp), nextValue);

    // Not the same timestamp as init timestamp, increment will be based on:
    //   tmp
    // NOTE: tmp was already changed above
    committableInt.incValue(nextTimestamp, increment);

    Timestamp otherTimestamp = std::max(0, distribution(gen));
    while (otherTimestamp == nextTimestamp) {
      otherTimestamp = distribution(gen);
    }
    // Any timestamp except nextTimestamp returns the committed value.
    EXPECT_EQ(committableInt.value(otherTimestamp), committedValue);
    // nextTimestamp returns the twice incremented value
    EXPECT_EQ(committableInt.value(nextTimestamp), nextValue + increment);
  }
}

TEST_F(CommittableIntTest, CommittableIntCommitValue) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int committedValue = distribution2(gen);

  CommittableInt committableInt = CommittableInt(initTimestamp, initValue);

  committableInt.commitValue(committedValue);

  EXPECT_EQ(committableInt.value(initTimestamp),
            initValue);  // TODO: shouldn't this be committedValue?
  EXPECT_EQ(committableInt.value(nextTimestamp), committedValue);
}

TEST_F(CommittableIntTest, CommittableIntCommit) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int committedValue = distribution2(gen);

  CommittableInt committableInt = CommittableInt(initTimestamp, initValue);

  EXPECT_EQ(committableInt.value(initTimestamp), initValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), initValue);

  committableInt.setValue(nextTimestamp, committedValue);

  EXPECT_EQ(committableInt.value(initTimestamp), initValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), committedValue);

  committableInt.commitIf(nextTimestamp);

  EXPECT_EQ(committableInt.value(initTimestamp), committedValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), committedValue);
}

TEST_F(CommittableIntTest, CommittableIntCommitIf) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int nextValue = distribution2(gen);

  CommittableInt committableInt = CommittableInt(initTimestamp, initValue);

  committableInt.commitIf(nextTimestamp);

  EXPECT_EQ(committableInt.value(initTimestamp), initValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), initValue);

  committableInt.commitIf(initTimestamp);

  EXPECT_EQ(committableInt.value(initTimestamp), initValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), initValue);

  committableInt.setValue(nextTimestamp, nextValue);

  committableInt.commitIf(nextTimestamp);

  EXPECT_EQ(committableInt.value(initTimestamp), nextValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), nextValue);

  committableInt.commitIf(initTimestamp);

  EXPECT_EQ(committableInt.value(initTimestamp), nextValue);
  EXPECT_EQ(committableInt.value(nextTimestamp), nextValue);
}

RC_GTEST_FIXTURE_PROP(CommittableIntTest, checkConstructorValue,
                      (Timestamp initTime, Int initValue, Timestamp anyTime)) {
  auto committableInt = CommittableInt(initTime, initValue);
  RC_ASSERT(committableInt.value(anyTime) == initValue);
}

RC_GTEST_FIXTURE_PROP(CommittableIntTest, checkSetValue,
                      (Timestamp initTime, Int initValue, Timestamp currentTime,
                       Int value)) {
  auto committableInt = CommittableInt(initTime, initValue);
  committableInt.setValue(currentTime, value);
  if (initTime != currentTime) {
    RC_ASSERT(committableInt.value(initTime) == initValue);
  }
}

RC_GTEST_FIXTURE_PROP(CommittableIntTest, checkCommittedValue,
                      (Timestamp initTime, Int initValue, Timestamp currentTime,
                       Int value)) {
  auto committableInt = CommittableInt(initTime, initValue);
  committableInt.setValue(currentTime, value);
  RC_ASSERT(committableInt.value(currentTime) == value);
}

}  // namespace