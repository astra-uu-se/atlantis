
#include <limits>
#include <random>
#include <vector>

#include "core/types.hpp"
#include "gtest/gtest.h"
#include "rapidcheck/gtest.h"
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
  Timestamp initTimestamp = std::max(0, distribution(gen));
  // Random inital value
  Int value = distribution(gen);

  SavedInt savedInt = SavedInt(initTimestamp, value);

  // Get the current value at the initial timestamp return the initial value
  EXPECT_EQ(savedInt.getValue(initTimestamp), value);

  // get at the current value at the initial timestamp. Returns the initial
  // value
  EXPECT_EQ(savedInt.getValue(initTimestamp), value);

  Timestamp otherTimestamp = std::max(0, distribution(gen));
  // Get the current value at another timestamp.
  // Should still return the initial value (as no other value has been
  // committed)
  EXPECT_EQ(savedInt.getValue(otherTimestamp), value);

  // get the current value at another timestamp. Should still return the
  // initial value (as no other value has been committed)
  EXPECT_EQ(savedInt.getValue(otherTimestamp), value);
}

TEST_F(SavedIntTest, SavedIntSetGetValue) {
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

  // Create a SavedInt
  SavedInt savedInt = SavedInt(initTimestamp, initValue);

  // Update the value for the other (next) timestamp
  savedInt.setValue(nextTimestamp, nextValue);

  // The value at the initial timestamp is still the initial value
  EXPECT_EQ(savedInt.getValue(initTimestamp), initValue);
  // The value at the next timestamp is now the next value
  EXPECT_EQ(savedInt.getValue(nextTimestamp), nextValue);

  Timestamp otherTimestamp = distribution2(gen);
  while (otherTimestamp == nextTimestamp) {
    otherTimestamp = distribution2(gen);
  }
  // The value for any timestamp except nextTimestamp is still the initial value
  EXPECT_EQ(savedInt.getValue(otherTimestamp), initValue);
}

TEST_F(SavedIntTest, SavedIntIncValue) {
  std::uniform_int_distribution<> distribution(
      std::numeric_limits<int>::min() + 10,
      std::numeric_limits<int>::max() - 10);

  Timestamp initTimestamp = std::max(0, distribution(gen));
  Int committedValue = distribution(gen);

  SavedInt savedInt = SavedInt(initTimestamp, committedValue);

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
    savedInt.incValue(nextTimestamp, increment);

    EXPECT_EQ(savedInt.getValue(initTimestamp), committedValue);
    EXPECT_EQ(savedInt.getValue(nextTimestamp), nextValue);

    // Not the same timestamp as init timestamp, increment will be based on:
    //   tmp
    // NOTE: tmp was already changed above
    savedInt.incValue(nextTimestamp, increment);

    Timestamp otherTimestamp = std::max(0, distribution(gen));
    while (otherTimestamp == nextTimestamp) {
      otherTimestamp = distribution(gen);
    }
    // Any timestamp except nextTimestamp returns the committed value.
    EXPECT_EQ(savedInt.getValue(otherTimestamp), committedValue);
    // nextTimestamp returns the twice incremented value
    EXPECT_EQ(savedInt.getValue(nextTimestamp), nextValue + increment);
  }
}

TEST_F(SavedIntTest, SavedIntCommitValue) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int committedValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTimestamp, initValue);

  savedInt.commitValue(committedValue);

  EXPECT_EQ(savedInt.getValue(initTimestamp),
            initValue);  // TODO: shouldn't this be committedValue?
  EXPECT_EQ(savedInt.getValue(nextTimestamp), committedValue);
}

TEST_F(SavedIntTest, SavedIntCommit) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int committedValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTimestamp, initValue);

  EXPECT_EQ(savedInt.getValue(initTimestamp), initValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), initValue);

  savedInt.setValue(nextTimestamp, committedValue);

  EXPECT_EQ(savedInt.getValue(initTimestamp), initValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), committedValue);

  savedInt.commitIf(nextTimestamp);

  EXPECT_EQ(savedInt.getValue(initTimestamp), committedValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), committedValue);
}

TEST_F(SavedIntTest, SavedIntCommitIf) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(),
                                                10000);
  std::uniform_int_distribution<> distribution2(
      10001, std::numeric_limits<int>::max());

  Timestamp initTimestamp = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTimestamp = distribution2(gen);
  Int nextValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTimestamp, initValue);

  savedInt.commitIf(nextTimestamp);

  EXPECT_EQ(savedInt.getValue(initTimestamp), initValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), initValue);

  savedInt.commitIf(initTimestamp);

  EXPECT_EQ(savedInt.getValue(initTimestamp), initValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), initValue);

  savedInt.setValue(nextTimestamp, nextValue);

  savedInt.commitIf(nextTimestamp);

  EXPECT_EQ(savedInt.getValue(initTimestamp), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), nextValue);

  savedInt.commitIf(initTimestamp);

  EXPECT_EQ(savedInt.getValue(initTimestamp), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTimestamp), nextValue);
}

RC_GTEST_FIXTURE_PROP(SavedIntTest, checkConstructorValue,
                      (Timestamp initTimestamp, Int initValue,
                       Timestamp anyTimestamp)) {
  auto savedInt = SavedInt(initTimestamp, initValue);
  RC_ASSERT(savedInt.getValue(anyTimestamp) == initValue);
}

RC_GTEST_FIXTURE_PROP(SavedIntTest, checkSetValue,
                      (Timestamp initTimestamp, Int initValue,
                       Timestamp currentTimestamp, Int value)) {
  auto savedInt = SavedInt(initTimestamp, initValue);
  savedInt.setValue(currentTimestamp, value);
  if (initTimestamp != currentTimestamp) {
    RC_ASSERT(savedInt.getValue(initTimestamp) == initValue);
  }
}

RC_GTEST_FIXTURE_PROP(SavedIntTest, checkCommittedValue,
                      (Timestamp initTimestamp, Int initValue,
                       Timestamp currentTimestamp, Int value)) {
  auto savedInt = SavedInt(initTimestamp, initValue);
  savedInt.setValue(currentTimestamp, value);
  RC_ASSERT(savedInt.getValue(currentTimestamp) == value);
}

}  // namespace