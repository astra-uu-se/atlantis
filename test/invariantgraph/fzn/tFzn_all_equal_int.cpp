#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_equal_int.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_all_equal_intTest : public FznTestBase {
 public:
  std::vector<std::string> inputs;
  std::string reified{"reified"};

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    for (size_t i = 0; i < inputs.size(); ++i) {
      const Int val = intVal(inputs.at(i), committedValue);
      for (size_t j = i + 1; j < inputs.size(); ++j) {
        if (val != intVal(inputs.at(j), committedValue)) {
          return false;
        }
      }
    }
    return true;
  }

  void generate() override {
    const size_t size = *rc::gen::inRange(1, 10);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier =
        isReified ? "fzn_all_equal_int_reif" : "fzn_all_equal_int";
    if (isReified) {
      addBoolArg(reified);
    } else {
      addBoolPar(reified, true);
    }
    generateConstraint();
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    std::optional<int> val{};
    size_t numUnfixed = 0;
    for (const auto& input : inputs) {
      if (!isFixed(input)) {
        ++numUnfixed;
        continue;
      }
      if (!val.has_value()) {
        val.emplace(intVal(input));
      }
      if (val.value() != intVal(input)) {
        return !boolVal(reified);
      }
    }
    if (val.has_value()) {
      for (const auto& input : inputs) {
        if (!isFixed(input)) {
          if (!varNodeConst(input).inDomain(Int{val.value()})) {
            return !boolVal(reified);
          }
        }
      }
    }
    return numUnfixed == inputs.size() ? boolVal(reified) : !boolVal(reified);
  }

  [[nodiscard]] bool neverSatisfied() const override {
    return !alwaysSatisfied();
  }

  [[nodiscard]] bool canMove() const override {
    return std::ranges::any_of(
        inputs, [&](const std::string& input) { return !isFixed(input); });
  }

  void move(bool committedValue) override {
    for (const auto& input : inputs) {
      if (!isFixed(input) && randBool()) {
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

RC_GTEST_FIXTURE_PROP(fzn_all_equal_intTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing