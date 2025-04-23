#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/bool_clause.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class bool_clauseTest : public FznTestBase {
 public:
  std::vector<std::string> inputsA{};
  std::vector<std::string> inputsB{};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const bool expected =
        std::ranges::any_of(inputsA,
                            [&](const auto& input) {
                              return boolVal(input, committedValue);
                            }) ||
        std::ranges::any_of(inputsB, [&](const auto& input) {
          return !boolVal(input, committedValue);
        });

    RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
    const bool shouldHold = violation(committedValue) == 0;
    return shouldHold == expected;
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    const bool alwaysSat =
        std::ranges::any_of(inputsA,
                            [&](const std::string& input) {
                              return isFixedTo(input, bool{true});
                            }) ||
        std::ranges::any_of(inputsB, [&](const std::string& input) {
          return isFixedTo(input, bool{false});
        });
    return alwaysSat;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    const bool alwaysUnsat =
        std::ranges::all_of(inputsA,
                            [&](const std::string& input) {
                              return !inDomain(input, bool{true});
                            }) &&
        std::ranges::all_of(inputsB, [&](const std::string& input) {
          return !inDomain(input, bool{false});
        });
    return alwaysUnsat;
  }

  void generate() override {
    constraintIdentifier = "bool_clause";
    const size_t sizeA = *rc::gen::inRange(0, 3);
    const size_t sizeB = *rc::gen::inRange(0, 3);
    inputsA.reserve(sizeA);
    for (size_t i = 0; i < sizeA; i++) {
      inputsA.emplace_back("b_a_" + std::to_string(i));
    }
    inputsB.reserve(sizeB);
    for (size_t i = 0; i < sizeB; i++) {
      inputsB.emplace_back("b_b_" + std::to_string(i));
    }
    addBoolVarArray(inputsA, "b_arr_a");
    addBoolVarArray(inputsB, "b_arr_b");
    generateConstraint();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(inputsA,
                               [&](const std::string& input) {
                                 return varId(input) != propagation::NULL_ID;
                               }) ||
           std::ranges::any_of(inputsB, [&](const std::string& input) {
             return varId(input) != propagation::NULL_ID;
           });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputsA) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
    for (const auto& input : inputsB) {
      if (varId(input) != propagation::NULL_ID && randBool()) {
        changeValue(input, committedValue);
      }
    }
  }

  void query() override { _solver->query(totalViolationVarId()); }
};

RC_GTEST_FIXTURE_PROP(bool_clauseTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
