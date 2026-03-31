#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "atlantis/types.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::testing {

#if defined(__clang__) || defined(__GNUC__)

RC_GTEST_PROP(Overflow, add, (Int lhs, Int rhs)) {
  Int expectedResult;
  const bool expected = __builtin_add_overflow(lhs, rhs, &expectedResult);
  Int actualResult;
  const bool actual = overflow::addOverflow(lhs, rhs, &actualResult);
  RC_ASSERT(expected == actual);
  if (!expected) {
    RC_ASSERT(expectedResult == actualResult);
  }
}

RC_GTEST_PROP(Overflow, sub, (Int lhs, Int rhs)) {
  Int expectedResult;
  const bool expected = __builtin_sub_overflow(lhs, rhs, &expectedResult);
  Int actualResult;
  const bool actual = overflow::subOverflow(lhs, rhs, &actualResult);
  RC_ASSERT(expected == actual);
  if (!expected) {
    RC_ASSERT(expectedResult == actualResult);
  }
}

RC_GTEST_PROP(Overflow, mul, (Int lhs, Int rhs)) {
  Int expectedResult;
  const bool expected = __builtin_mul_overflow(lhs, rhs, &expectedResult);
  Int actualResult;
  const bool actual = overflow::mulOverflow(lhs, rhs, &actualResult);
  RC_ASSERT(expected == actual);
  if (!expected) {
    RC_ASSERT(expectedResult == actualResult);
  }
}

#endif

}  // namespace atlantis::testing
