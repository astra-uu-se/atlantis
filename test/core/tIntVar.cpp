#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include "core/intVar.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"

namespace {
class IntVarTest : public ::testing::Test {
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

TEST_F(IntVarTest, SavedIntConstructor) {
  std::uniform_int_distribution<> distribution(std::numeric_limits<int>::min(),
                                               std::numeric_limits<int>::max());

  Int lowerBound = -10;
  Int upperBound = 10;

  // Random timestamp
  VarId varId = distribution(gen);

  auto intVarNoValue = IntVar(varId, lowerBound, upperBound);

  Timestamp timestamp = distribution(gen);
  // The value defaults to 0
  ASSERT_EQ(intVarNoValue.getValue(timestamp), 0);
  ASSERT_EQ(intVarNoValue.getCommittedValue(), 0);

  // default timestamp is NULL_TIMESTAMP
  ASSERT_EQ(intVarNoValue.getTmpTimestamp(), NULL_TIMESTAMP);

  ASSERT_FALSE(intVarNoValue.hasChanged(timestamp));

  // Random inital value
  Int value = distribution(gen);

  IntVar intVarWithValue = IntVar(varId, value, -10, 10);

  ASSERT_EQ(intVarWithValue.getValue(timestamp), value);
  ASSERT_EQ(intVarWithValue.getCommittedValue(), value);

  // default timestamp is zero
  ASSERT_EQ(intVarWithValue.getTmpTimestamp(), NULL_TIMESTAMP);

  ASSERT_FALSE(intVarWithValue.hasChanged(timestamp));

  EXPECT_THROW(IntVar(varId, value, upperBound, lowerBound), std::out_of_range);
}

TEST_F(IntVarTest, InDomain) {
  Int lowerBound = -10;
  Int upperBound = 10;
  Timestamp timestamp = Timestamp(1);
  IntVar intVar = IntVar(timestamp, 0, lowerBound, upperBound);

  for (Int value = lowerBound; value <= upperBound; ++value) {
    EXPECT_TRUE(intVar.inDomain(value));
  }

  for (Int value = lowerBound - 1000; value < lowerBound; ++value) {
    EXPECT_FALSE(intVar.inDomain(value));
  }

  for (Int value = upperBound + 1; value < upperBound + 1000; ++value) {
    EXPECT_FALSE(intVar.inDomain(value));
  }
}

TEST_F(IntVarTest, UpdateDomain) {
  Int initialLowerBound = 0;
  Int initialUpperBound = 0;

  Timestamp timestamp = Timestamp(1);
  IntVar intVar = IntVar(timestamp, 0, initialLowerBound, initialUpperBound);

  for (Int value = 1; value <= 1000; ++value) {
    EXPECT_FALSE(intVar.inDomain(-value));
    EXPECT_FALSE(intVar.inDomain(value));
    intVar.updateDomain(-value, value);
    EXPECT_TRUE(intVar.inDomain(-value));
    EXPECT_TRUE(intVar.inDomain(value));
  }

  EXPECT_THROW(intVar.updateDomain(10, -10), std::out_of_range);
}

}  // namespace