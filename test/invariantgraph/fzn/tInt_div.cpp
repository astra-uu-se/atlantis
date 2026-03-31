#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/int_div.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

static Int div_ceil(Int n, Int d) { return n / d + (n % d > 0 ? 1 : 0); }

static Int div_floor(Int n, Int d) { return n / d - (n % d < 0 ? 1 : 0); }

class int_divTest : public FznTestBase {
 public:
  std::string numerator{"numerator"};
  std::string denominator{"denominator"};
  std::string quotient{"quotient"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        intVal(denominator) == 0
            ? false
            : intVal(numerator) / intVal(denominator) == intVal(quotient);

    const bool isSolution = violation(committedValue) == 0;
    return isSolution ? expected : !expected;
  }

  void generate() override {
    addIntArg(numerator);
    addIntArg(denominator);
    addIntArg(quotient);
    constraintIdentifier = "int_div";
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixedTo(denominator, Int{0})) {
      return false;
    }
    if (isFixedTo(numerator, Int{0})) {
      return isFixedTo(quotient, Int{0});
    }
    if (isFixedTo(denominator, Int{1})) {
      return varNodeId(numerator) == varNodeId(quotient);
    }
    if (isFixed(numerator) && isFixed(denominator) && isFixed(quotient)) {
      return intVal(numerator) / intVal(denominator) == intVal(quotient);
    }
    if (isFixedTo(quotient, Int{0})) {
      if (isFixed(numerator)) {
        const Int nLb = std::min(-intVal(numerator), intVal(numerator));
        const Int nUb = std::max(-intVal(numerator), intVal(numerator));
        const Int dLb =
            std::min(lowerBound(denominator), -upperBound(denominator));
        const Int dUb =
            std::min(-lowerBound(denominator), upperBound(denominator));
        if (nLb < dUb || dLb < nUb) {
          return true;
        }
      }
      if (isFixed(denominator)) {
        const Int nLb = std::min(lowerBound(numerator), -upperBound(numerator));
        const Int nUb = std::max(-lowerBound(numerator), upperBound(numerator));
        const Int dLb = std::min(intVal(denominator), -intVal(denominator));
        const Int dUb = std::min(-intVal(denominator), intVal(denominator));
        if (nLb < dUb || dLb < nUb) {
          return true;
        }
      }
    }

    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (isFixedTo(denominator, Int{0})) {
      return true;
    }
    if (isFixedTo(numerator, Int{0})) {
      return !inDomain(quotient, Int{0});
    }
    if (isFixedTo(quotient, Int{0})) {
      const Int nLb = std::min(lowerBound(numerator), -upperBound(numerator));
      const Int nUb = std::max(-lowerBound(numerator), upperBound(numerator));
      const Int dLb =
          std::min(lowerBound(denominator), -upperBound(denominator));
      const Int dUb =
          std::max(-lowerBound(denominator), upperBound(denominator));

      if (dUb <= nLb || nUb <= dLb) {
        return true;
      }
    }
    if (isFixed(numerator) && isFixed(denominator) && isFixed(quotient)) {
      return intVal(numerator) / intVal(denominator) != intVal(quotient);
    }
    if (isFixed(numerator) && isFixed(quotient) && intVal(quotient) != 0) {
      const Int nVal = intVal(numerator);
      const Int qVal = intVal(quotient);
      RC_ASSERT(qVal != Int{0});
      const Int dLb = lowerBound(denominator);
      const Int dUb = upperBound(denominator);
      const auto vals = std::array{div_floor(nVal, qVal), div_ceil(nVal, qVal),
                                   div_floor(nVal, qVal), div_ceil(nVal, qVal)};
      const Int expectedLb = std::ranges::min(vals);
      const Int expectedUb = std::ranges::max(vals);
      if (expectedUb < dLb || dUb < expectedLb) {
        return true;
      }
    }
    if (isFixed(denominator) && isFixed(quotient)) {
      const Int qVal = intVal(quotient);
      const Int dVal = intVal(denominator);

      const auto arr =
          std::array{std::pair{qVal - (qVal % dVal == 0 ? 0 : 1), dVal},
                     std::pair{qVal + (qVal % dVal == 0 ? 0 : 1), dVal}};
      Int expectedLb = std::numeric_limits<Int>::max();
      Int expectedUb = std::numeric_limits<Int>::min();
      for (const auto& [q, d] : arr) {
        Int prod;
        if (overflow::mulOverflow(q, d, &prod)) {
          if ((q >= 0) == (d >= 0)) {
            expectedUb = std::numeric_limits<Int>::max();
          } else {
            expectedLb = std::numeric_limits<Int>::min();
          }
          continue;
        }
        expectedLb = std::min(expectedLb, prod);
        expectedUb = std::max(expectedUb, prod);
      }

      if (expectedUb < lowerBound(numerator) ||
          upperBound(numerator) < expectedLb) {
        return true;
      }
    }
    const Int nLb = lowerBound(numerator);
    const Int nUb = upperBound(numerator);
    const Int dLb = lowerBound(denominator);
    const Int dUb = upperBound(denominator);
    const auto arr = std::array{std::pair{nLb, dLb},
                                std::pair{nLb, dUb},
                                std::pair{nLb, dLb < 0 && 0 < dUb ? -1 : dLb},
                                std::pair{nLb, dLb < 0 && 0 < dUb ? 1 : dLb},
                                std::pair{nUb, dLb},
                                std::pair{nUb, dUb},
                                std::pair{nUb, dLb < 0 && 0 < dUb ? -1 : dLb},
                                std::pair{nUb, dLb < 0 && 0 < dUb ? 1 : dLb}};
    Int expectedLb = std::numeric_limits<Int>::max();
    Int expectedUb = std::numeric_limits<Int>::min();
    for (const auto& [n, d] : arr) {
      expectedLb = std::ranges::min(
          std::array{expectedLb, div_floor(n, d), div_ceil(n, d)});
      expectedUb = std::ranges::max(
          std::array{expectedUb, div_floor(n, d), div_ceil(n, d)});
    }
    const Int qLb = lowerBound(quotient);
    const Int qUb = upperBound(quotient);
    if (expectedUb < qLb || qUb < expectedLb) {
      return true;
    }

    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(
        std::array{varId(numerator), varId(denominator)},
        [&](const auto vId) { return vId != propagation::NULL_ID; });
  }

  void move(bool committedValue) override {
    if (varId(numerator) != propagation::NULL_ID && randBool()) {
      changeValue(numerator, committedValue);
    }
    if (varId(denominator) != propagation::NULL_ID && randBool()) {
      changeValue(denominator, committedValue);
    }
  }

  void query() override {
    if (varId(quotient) != propagation::NULL_ID) {
      _solver->query(varId(quotient));
    }
    _solver->query(totalViolationVarId());
  }
};

RC_GTEST_FIXTURE_PROP(int_divTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
