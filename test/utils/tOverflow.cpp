#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "atlantis/types.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::testing {

#if defined(__clang__) || defined(__GNUC__)

class OverflowTest : public ::testing::Test {
 public:
  std::vector<std::pair<Int, Int>> extrema;

  void SetUp() override {
    std::vector<Int> limits{std::numeric_limits<Int>::min(), -1, 0, 1,
                            std::numeric_limits<Int>::max()};
    extrema.clear();
    extrema.resize(limits.size() * limits.size());
    for (const Int lhs : limits) {
      for (const Int rhs : limits) {
        extrema.emplace_back(lhs, rhs);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(OverflowTest, add, (Int lhs, Int rhs)) {
  Int expectedResult;
  const bool expected = __builtin_add_overflow(lhs, rhs, &expectedResult);
  Int actualResult;
  const bool actual = overflow::addOverflow(lhs, rhs, &actualResult);
  RC_ASSERT(expected == actual);
  if (!expected) {
    RC_ASSERT(expectedResult == actualResult);
  }
}

RC_GTEST_FIXTURE_PROP(OverflowTest, sub, (Int lhs, Int rhs)) {
  Int expectedResult;
  const bool expected = __builtin_sub_overflow(lhs, rhs, &expectedResult);
  Int actualResult;
  const bool actual = overflow::subOverflow(lhs, rhs, &actualResult);
  RC_ASSERT(expected == actual);
  if (!expected) {
    RC_ASSERT(expectedResult == actualResult);
  }
}

RC_GTEST_FIXTURE_PROP(OverflowTest, mul, (Int lhs, Int rhs)) {
  Int expectedResult;
  const bool expected = __builtin_mul_overflow(lhs, rhs, &expectedResult);
  Int actualResult;
  const bool actual = overflow::mulOverflow(lhs, rhs, &actualResult);
  RC_ASSERT(expected == actual);
  if (!expected) {
    RC_ASSERT(expectedResult == actualResult);
  }
}

TEST_F(OverflowTest, add_extrema) {
  for (const auto& [lhs, rhs] : extrema) {
    Int expectedResult;
    const bool expected = __builtin_add_overflow(lhs, rhs, &expectedResult);
    Int actualResult;
    const bool actual = overflow::addOverflow(lhs, rhs, &actualResult);
    EXPECT_EQ(expected, actual);
    if (!expected) {
      EXPECT_EQ(expectedResult, actualResult);
    }
  }
}

TEST_F(OverflowTest, sub_extrema) {
  for (const auto& [lhs, rhs] : extrema) {
    Int expectedResult;
    const bool expected = __builtin_sub_overflow(lhs, rhs, &expectedResult);
    Int actualResult;
    const bool actual = overflow::subOverflow(lhs, rhs, &actualResult);
    EXPECT_EQ(expected, actual);
    if (!expected) {
      EXPECT_EQ(expectedResult, actualResult);
    }
  }
}

TEST_F(OverflowTest, mul_extrema) {
  for (const auto& [lhs, rhs] : extrema) {
    Int expectedResult;
    const bool expected = __builtin_mul_overflow(lhs, rhs, &expectedResult);
    Int actualResult;
    const bool actual = overflow::mulOverflow(lhs, rhs, &actualResult);
    EXPECT_EQ(expected, actual);
    if (!expected) {
      EXPECT_EQ(expectedResult, actualResult);
    }
  }
}

#endif

}  // namespace atlantis::testing
