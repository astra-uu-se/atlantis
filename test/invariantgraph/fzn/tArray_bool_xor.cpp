#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_xor.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_bool_xorTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const auto& input) { return boolVal(input, committedValue); });
    const bool expected = numTrue == 1;
    const bool actual = boolVal(reified, committedValue);

    if (isFixed(reified)) {
      RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
      const bool shouldHold = violation(committedValue) == 0;
      return shouldHold ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  void generate() override {
    constraintIdentifier = "array_bool_xor";
    const size_t size = *rc::gen::inRange(0, 3);
    inputs.reserve(size);
    for (size_t i = 0; i < size; i++) {
      inputs.emplace_back("b_" + std::to_string(i));
    }
    addBoolVarArray(inputs);
    addBoolPar(reified, true);
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

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(reified));
  }
};

RC_GTEST_FIXTURE_PROP(array_bool_xorTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing