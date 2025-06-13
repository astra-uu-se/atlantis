#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include <string>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/fzn_count_lt.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class fzn_count_ltTest : public FznTestBase {
 public:
  std::vector<std::string> inputs{};
  std::string needle{"needle"};
  std::string output{"output"};
  std::string reified{"reified"};

  [[nodiscard]] std::pair<Int, Int> getBounds() const {
    Int lb = 0;
    Int ub = 0;
    for (const auto& input : inputs) {
      if (isFixed(input)) {
        const Int inputVal = intVal(input);
        if (isFixed(needle)) {
          if (inputVal == intVal(needle)) {
            ++lb;
            ++ub;
          }
        } else if (inDomain(needle, inputVal)) {
          ++ub;
        }
      } else if (isFixed(needle)) {
        if (inDomain(input, intVal(needle))) {
          ++ub;
        }
      } else if (!varNodeConst(input).constDomain()->isDisjoint(
                     *varNodeConst(needle).constDomain())) {
        ++ub;
      }
    }
    return {lb, ub};
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    Int count = 0;
    for (const auto& input : inputs) {
      if (intVal(input, committedValue) == intVal(needle, committedValue)) {
        ++count;
      }
    }

    const bool expected = count > intVal(output, committedValue);
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
      return lb > upperBound(output);
    }
    return lowerBound(output) >= ub;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(reified)) {
      return false;
    }
    const auto [lb, ub] = getBounds();
    if (isFixedTo(reified, true)) {
      return lowerBound(output) >= ub;
    }
    return lb > upperBound(output);
  }

  void generate() override {
    const size_t size = *rc::gen::inRange<size_t>(0, 4);
    inputs.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    addIntVarArray(inputs);
    addIntArg(needle);
    addIntArg(output);

    const bool isReified = *rc::gen::arbitrary<bool>();
    constraintIdentifier = isReified ? "fzn_count_lt_reif" : "fzn_count_lt";
    if (isReified) {
      addBoolArg(reified);
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

  void move(bool committedValue) override {
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
         std::array{varId(reified), varId(output), totalViolationVarId()}) {
      if (vId != propagation::NULL_ID) {
        _solver->query(vId);
      }
    }
  }
};

RC_GTEST_FIXTURE_PROP(fzn_count_ltTest, RapidCheck, ()) { rapidCheck(); }

}  // namespace atlantis::testing
