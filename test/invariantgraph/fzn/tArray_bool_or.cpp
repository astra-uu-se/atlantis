#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_or.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_bool_orTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::string output{"output"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected = std::ranges::any_of(inputs, [&](const auto& input) {
      return boolVal(input, committedValue);
    });
    const bool actual = boolVal(output, committedValue);

    RC_ASSERT(isFixed(output) ==
              (totalViolationVarId() != propagation::NULL_ID));

    if (totalViolationVarId() != propagation::NULL_ID) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const bool alwaysUnsat =
        std::ranges::all_of(inputs, [&](const std::string& input) {
          return isFixedTo(input, bool{false});
        });
    if (alwaysUnsat) {
      return inDomain(output, bool{false});
    }
    const bool alwaysSat = std::ranges::any_of(
        inputs,
        [&](const std::string& input) { return isFixedTo(input, bool{true}); });
    if (alwaysSat) {
      return inDomain(output, bool{true});
    }
    if (!isFixed(output)) {
      VarNodeId unfixedVar = NULL_NODE_ID;
      for (const std::string& input : inputs) {
        if (!isFixed(input)) {
          if (unfixedVar == NULL_NODE_ID) {
            unfixedVar = varNodeId(input);
          } else {
            unfixedVar = NULL_NODE_ID;
            break;
          }
        }
      }
      if (unfixedVar != NULL_NODE_ID) {
        return varNodeId(output) == unfixedVar;
      }
    }
    return false;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(output)) {
      return false;
    }
    const bool alwaysUnsat = std::ranges::all_of(
        inputs,
        [&](const std::string& input) { return !inDomain(input, bool{true}); });
    if (alwaysUnsat) {
      return !inDomain(output, bool{false});
    }
    const bool alwaysSat =
        std::ranges::any_of(inputs, [&](const std::string& input) {
          return !inDomain(input, bool{false});
        });
    if (alwaysSat) {
      return !inDomain(output, bool{true});
    }
    return false;
  }

  void generate() override {
    constraintIdentifier = "array_bool_or";
    const size_t size = *rc::gen::inRange(0, 3);
    inputs.reserve(size);
    for (size_t i = 0; i < size; i++) {
      inputs.emplace_back("b_" + std::to_string(i));
    }
    addBoolVarArray(inputs);
    addBoolArg(output);
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::any_of(inputs.begin(), inputs.end(),
                       [&](const std::string& input) {
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
                       : varId(output));
  }
};

RC_GTEST_FIXTURE_PROP(array_bool_orTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
