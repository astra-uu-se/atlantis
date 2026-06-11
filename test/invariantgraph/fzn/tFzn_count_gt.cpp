#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_gt.hpp"
#include "atlantis/utils/domains.hpp"
#include "tFzn_count.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_count_gtTest : public fzn_countTest {
 public:
  [[nodiscard]] bool isSatisfied(const bool committedValue) const override {
    Int count = 0;
    const Int needleVal = intVal(needle, committedValue);
    for (const auto& input : inputs) {
      const Int inputVal = intVal(input, committedValue);
      if (inputVal == needleVal) {
        ++count;
        RC_LOG() << input << ": " << needleVal << std::endl;
      }
    }

    const Int boundVal = intVal(bound, committedValue);

    RC_LOG() << bound << " > count" << ": " << boundVal << " > " << count
             << std::endl;

    const bool expected = boundVal > count;
    const bool actual = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      const bool satAssignment = violation(committedValue) == 0;
      return satAssignment ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    if (isFixedTo(reified, true)) {
      return ub < lowerBound(bound);
    }
    return upperBound(bound) <= lb;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    if (isFixedTo(reified, true)) {
      return upperBound(bound) <= lb;
    }
    return ub < lowerBound(bound);
  }

  void generate() override {
    const size_t size = *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
    addIntArg(needle);
    addIntArg(bound);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_count_gt_reif" : "fzn_count_gt";
    if (isReified) {
      addBoolArg(BoolArgState::VAR, reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return varId(needle) != propagation::NULL_ID ||
           std::ranges::any_of(inputs, [&](const std::string& input) {
             return varId(input) != propagation::NULL_ID;
           });
  }

  void move(const bool committedValue) override {
    if (varId(needle) != propagation::NULL_ID && randBool()) {
      changeValue(needle, committedValue);
    }
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override {
    for (const auto& vId :
         std::array{varId(reified), varId(bound), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_count_gtTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
