#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_xor.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_xorTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{"b_1", "b_2"};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const auto& input) { return boolVal(input, committedValue); });
    const bool expected = numTrue == 1;
    const bool shouldHold = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
      const bool actual = violation(committedValue) == 0;
      return shouldHold ? expected == actual : expected != actual;
    }
    return shouldHold ? expected : !expected;
  }

  void generate() override {
    constraintIdentifier = "bool_xor";
    addBoolArg(inputs.front());
    addBoolArg(inputs.back());
    const bool isReified = *rc::gen::arbitrary<bool>();
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      const size_t numFalse =
          std::ranges::count_if(inputs, [&](const std::string& input) {
            return isFixedTo(input, bool{false});
          });
      return numFalse + 1 == inputs.size();
    }

    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const std::string& input) { return isFixedTo(input, bool{true}); });

    return boolVal(reified) ? numTrue == 1 : numTrue != 1;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }

    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const std::string& input) { return isFixedTo(input, bool{true}); });

    const size_t numFalse =
        std::ranges::count_if(inputs, [&](const std::string& input) {
          return isFixedTo(input, bool{false});
        });

    return boolVal(reified) ? (numTrue > 1 || numFalse == inputs.size())
                            : (numTrue == 1 && (numFalse + 1 == inputs.size()));
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputs, [&](const std::string& input) {
      return varId(input) != propagation::NULL_ID;
    });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override { _solver->query(totalViolationVarId()); }
};

RC_GTEST_FIXTURE_PROP(bool_xorTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing