#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_var_int_element.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_var_int_elementTest : public FznTestBase {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  std::string idx{"idx"};
  Int offset{1};
  std::string output{"output"};
  std::vector<std::string> inputs{};

  void generate() override {
    const Int size = *rc::gen::inRange(1, 10);
    const bool useOffset = *rc::gen::arbitrary<bool>();

    constraintIdentifier = useOffset ? "array_var_int_element_offset"
                                     : "array_var_int_element_nonshifted";
    for (Int i = 0; i < size; i++) {
      inputs.emplace_back("i_" + std::to_string(i));
    }
    const Int lb =
        std::vector<Int>{-1024, -1, 0, 1, 1024}.at(*rc::gen::inRange(0, 5));
    addIntArg(lb, size + lb - 1, idx);

    offset = lowerBound(idx);
    addIntVarArray(inputs);
    addIntArg(output);
    if (useOffset) {
      addArg(offset);
    }
    generateConstraint();
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const Int idxVal = intVal(idx, committedValue);
    const Int expected = intVal(inputs.at(idxVal - offset));
    const Int actual = intVal(output, committedValue);

    if (isFixed(output)) {
      const bool isSolution = violation(committedValue) == 0;
      return isSolution ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(output)) {
      return false;
    }
    const Int outVal = intVal(output);
    if (isFixed(idx)) {
      const auto& input = inputs.at(intVal(idx) - offset);
      return isFixed(input) && intVal(input) != outVal;
    }
    const auto& idxNode = varNodeConst(idx);
    return std::none_of(idxNode.constDomain()->begin(),
                        idxNode.constDomain()->end(), [&](const Int val) {
                          const auto& input = inputs.at(val - offset);
                          return isFixed(input)
                                     ? intVal(input) == outVal
                                     : varNodeConst(input).inDomain(outVal);
                        });
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (isFixed(idx)) {
      const auto& input = inputs.at(intVal(idx) - offset);
      if (isFixed(input) && isFixed(output)) {
        return intVal(input) == intVal(output);
      }
      if (!isFixed(input) && !isFixed(output)) {
        return varNodeId(input) == varNodeId(output);
      }
      return false;
    }
    const auto& dom = varNodeConst(idx).constDomain();
    if (isFixed(output)) {
      const Int outVal = intVal(output);
      return std::all_of(dom->begin(), dom->end(), [&](const Int val) {
        const auto& input = inputs.at(val - offset);
        return isFixed(input) && intVal(input) == outVal;
      });
    }
    return false;
  }

  [[nodiscard]] bool canMove() const override {
    return varId(idx) != propagation::NULL_ID ||
           std::ranges::any_of(inputs, [&](const auto& input) {
             return varId(input) != propagation::NULL_ID;
           });
  }

  void move(bool committedValue) override {
    if (randBool()) {
      changeValue(idx, committedValue);
    }
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

RC_GTEST_FIXTURE_PROP(array_var_int_elementTest, RapidCheck, ()) {
  rapidCheck();
}
}  // namespace atlantis::testing
