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

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const auto& input) { return boolVal(input, committedValue); });
    const bool expected = numTrue == 1;

    RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
    return expected == (violation(committedValue) == 0);
  }

  void SetUp() override {
    FznTestBase::SetUp();
    constraintIdentifier = "array_bool_xor";
  }

  void generate() override {
    const size_t size = *rc::gen::inRange(0, 3);
    inputs.reserve(size);
    for (size_t i = 0; i < size; i++) {
      inputs.emplace_back("b_" + std::to_string(i));
    }
    addBoolVarArray(inputs);
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const std::string& input) { return !inDomain(input, bool{true}); });

    if (numTrue == 1) {
      return true;
    }

    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    const size_t numTrue = std::ranges::count_if(
        inputs,
        [&](const std::string& input) { return isFixedTo(input, bool{true}); });

    const size_t numFalse =
        std::ranges::count_if(inputs, [&](const std::string& input) {
          return isFixedTo(input, bool{false});
        });

    if (numTrue > 1 || numFalse == inputs.size()) {
      return true;
    }

    return false;
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

RC_GTEST_FIXTURE_PROP(array_bool_xorTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing