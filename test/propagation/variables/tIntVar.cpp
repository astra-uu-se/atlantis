#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IntVarTest : public ::testing::Test {
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

TEST_F(IntVarTest, CommittableIntConstructor) {
  int lowerBound = std::numeric_limits<int>::min();
  int upperBound = std::numeric_limits<int>::max();
  std::uniform_int_distribution<> distribution(lowerBound, upperBound);

  // Random timestamp
  VarId varId = distribution(gen);

  auto intVarNoValue = IntVar(varId, lowerBound, upperBound);

  Timestamp timestamp = distribution(gen);
  // The value defaults to 0
  ASSERT_EQ(intVarNoValue.value(timestamp), 0);
  ASSERT_EQ(intVarNoValue.committedValue(), 0);

  // default timestamp is NULL_TIMESTAMP
  ASSERT_EQ(intVarNoValue.tmpTimestamp(), NULL_TIMESTAMP);

  ASSERT_FALSE(intVarNoValue.hasChanged(timestamp));

  // Random inital value
  Int value = distribution(gen);

  IntVar intVarWithValue = IntVar(varId, value, lowerBound, upperBound);

  ASSERT_EQ(intVarWithValue.value(timestamp), value);
  ASSERT_EQ(intVarWithValue.committedValue(), value);

  // default timestamp is zero
  ASSERT_EQ(intVarWithValue.tmpTimestamp(), NULL_TIMESTAMP);

  ASSERT_FALSE(intVarWithValue.hasChanged(timestamp));

  EXPECT_THROW(IntVar(varId, value, upperBound, lowerBound), std::out_of_range);

  EXPECT_THROW(IntVar(varId, 10, -5, 5), std::out_of_range);
}

TEST_F(IntVarTest, InDomain) {
  Int lowerBound = -10;
  Int upperBound = 10;
  Timestamp timestamp(1);
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

TEST_F(IntVarTest, UpdateBounds) {
  Int initialLowerBound = 0;
  Int initialUpperBound = 0;

  Timestamp timestamp(1);
  IntVar intVar = IntVar(timestamp, 0, initialLowerBound, initialUpperBound);

  for (Int value = 1; value <= 1000; ++value) {
    EXPECT_FALSE(intVar.inDomain(-value));
    EXPECT_FALSE(intVar.inDomain(value));
    intVar.updateBounds(-value, value, false);
    EXPECT_TRUE(intVar.inDomain(-value));
    EXPECT_TRUE(intVar.inDomain(value));
  }

  EXPECT_THROW(intVar.updateBounds(10, -10, false), std::out_of_range);
}

}  // namespace atlantis::testing