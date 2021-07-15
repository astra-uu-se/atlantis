
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "core/types.hpp"
#include "variables/savedInt.hpp"

namespace {
class SavedIntTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(SavedIntTest, SavedIntConstructor) {
  std::uniform_int_distribution<> distribution(
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max() - 1);

  // Random timestamp
  Timestamp initTime = std::max(0, distribution(gen));
  // Random inital value
  Int value = distribution(gen);

  SavedInt savedInt = SavedInt(initTime, value);

  // Get the current value at the initial timestamp return the initial value
  EXPECT_EQ(savedInt.getValue(initTime), value);

  // get at the current value at the initial timestamp. Returns the initial
  // value
  EXPECT_EQ(savedInt.getValue(initTime), value);

  Timestamp otherTime = std::max(0, distribution(gen));
  // Get the current value at another timestamp.
  // Should still return the initial value (as no other value has been
  // committed)
  EXPECT_EQ(savedInt.getValue(otherTime), value);

  // get the current value at another timestamp. Should still return the
  // initial value (as no other value has been committed)
  EXPECT_EQ(savedInt.getValue(otherTime), value);
}

TEST_F(SavedIntTest, SavedIntSetGetValue) {
  // A uniform distribution for the initial value and timestamp
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  // A uniform distribution for the other (next) timestamp
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int nextValue = distribution2(gen);

  // Create a SavedInt
  SavedInt savedInt = SavedInt(initTime, initValue);

  // Update the value for the other (next) timestamp
  savedInt.setValue(nextTime, nextValue);

  // The value at the initial timestamp is still the initial value
  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  // The value at the next timestamp is now the next value
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

  Timestamp otherTime = distribution2(gen);
  while (otherTime == nextTime) {
    otherTime = distribution2(gen);
  }
  // The value for any timestamp except nextTime is still the initial value
  EXPECT_EQ(savedInt.getValue(otherTime), initValue);
}

TEST_F(SavedIntTest, SavedIntIncValue) {
  std::uniform_int_distribution<> distribution(
      std::numeric_limits<int>::min() + 10,
      std::numeric_limits<int>::max() - 10);

  Timestamp initTime = std::max(0, distribution(gen));
  Int committedValue = distribution(gen);

  SavedInt savedInt = SavedInt(initTime, committedValue);

  Timestamp nextTime;
  Int nextValue;
  // Increase -10, -9, ..., -1, 1, 2, ..., 10
  for (Int increment = -10; increment <= 10; ++increment) {
    if (increment == 0) {
      continue;
    }
    nextTime = initTime + increment;
    nextValue = committedValue + increment;

    // Not the same timestamp as init timestamp, increment will be based on:
    //   saved
    savedInt.incValue(nextTime, increment);

    EXPECT_EQ(savedInt.getValue(initTime), committedValue);
    EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

    // Not the same timestamp as init timestamp, increment will be based on:
    //   tmp
    // NOTE: tmp was already changed above
    savedInt.incValue(nextTime, increment);

    Timestamp otherTime = std::max(0, distribution(gen));
    while (otherTime == nextTime) {
      otherTime = distribution(gen);
    }
    // Any time except nextTime returns the committed value.
    EXPECT_EQ(savedInt.getValue(otherTime), committedValue);
    // nextTime returns the twice incremented value
    EXPECT_EQ(savedInt.getValue(nextTime), nextValue + increment);
  }
}

TEST_F(SavedIntTest, SavedIntCommitValue) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int committedValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTime, initValue);

  savedInt.commitValue(committedValue);

  EXPECT_EQ(savedInt.getValue(initTime),
            initValue);  // TODO: shouldn't this be committedValue?
  EXPECT_EQ(savedInt.getValue(nextTime), committedValue);
}

TEST_F(SavedIntTest, SavedIntCommit) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int committedValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTime, initValue);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);

  savedInt.setValue(nextTime, committedValue);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), committedValue);

  savedInt.commitIf(nextTime);

  EXPECT_EQ(savedInt.getValue(initTime), committedValue);
  EXPECT_EQ(savedInt.getValue(nextTime), committedValue);
}

TEST_F(SavedIntTest, SavedIntCommitIf) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int nextValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTime, initValue);

  savedInt.commitIf(nextTime);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);

  savedInt.commitIf(initTime);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);

  savedInt.setValue(nextTime, nextValue);

  savedInt.commitIf(nextTime);

  EXPECT_EQ(savedInt.getValue(initTime), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

  savedInt.commitIf(initTime);

  EXPECT_EQ(savedInt.getValue(initTime), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);
}

RC_GTEST_FIXTURE_PROP(SavedIntTest, checkConstructorValue,
                      (Timestamp initTime, Int initValue, Timestamp anyTime)) {
  auto savedInt = SavedInt(initTime, initValue);
  RC_ASSERT(savedInt.getValue(anyTime) == initValue);
}

RC_GTEST_FIXTURE_PROP(SavedIntTest, checkSetValue,
                      (Timestamp initTime, Int initValue, Timestamp currentTime,
                       Int value)) {
  auto savedInt = SavedInt(initTime, initValue);
  savedInt.setValue(currentTime, value);
  if (initTime != currentTime) {
    RC_ASSERT(savedInt.getValue(initTime) == initValue);
  }
}

RC_GTEST_FIXTURE_PROP(SavedIntTest, checkCommittedValue,
                      (Timestamp initTime, Int initValue, Timestamp currentTime,
                       Int value)) {
  auto savedInt = SavedInt(initTime, initValue);
  savedInt.setValue(currentTime, value);
  RC_ASSERT(savedInt.getValue(currentTime) == value);
}

}  // namespace