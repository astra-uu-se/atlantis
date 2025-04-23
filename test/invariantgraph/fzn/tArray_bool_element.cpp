#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <vector>

#include "./fznTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_bool_element.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class array_bool_elementTest : public FznTestBase {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  std::string idx{"idx"};
  Int offset{1};
  std::string output{"output"};
  std::vector<bool> parameters{};

  void generate() override {
    const Int size = *rc::gen::inRange(1, 10);
    const bool useOffset = *rc::gen::arbitrary<bool>();

    constraintIdentifier =
        useOffset ? "array_bool_element_offset" : "array_bool_element";
    parameters = *rc::gen::container<std::vector<bool>>(
        size, rc::gen::arbitrary<bool>());
    const Int lb =
        std::vector<Int>{-1024, -1, 0, 1, 1024}.at(*rc::gen::inRange(0, 5));
    addIntArg(lb, size + lb - 1, idx);

    offset = lowerBound(idx);
    addArg(parameters);
    addBoolArg(output);
    if (useOffset) {
      addArg(offset);
    }
    generateConstraint();
  }

  [[nodiscard]] bool isSatisfied(bool committedValue) const override {
    const Int idxVal = intVal(idx, committedValue);
    const bool expected = parameters.at(idxVal - offset);
    const bool actual = boolVal(output, committedValue);

    if (isFixed(output)) {
      RC_ASSERT(totalViolationVarId() != propagation::NULL_ID);
      const bool shouldHold = violation(committedValue) == 0;
      return shouldHold ? expected == actual : expected != actual;
    }
    return expected == actual;
  }

  [[nodiscard]] bool neverSatisfied() const override {
    if (!isFixed(idx) && !isFixed(output)) {
      const auto& idxNode = _invariantGraph->varNode(idx);
      const auto& outputNode = _invariantGraph->varNode(output);
      return std::none_of(idxNode.constDomain()->begin(),
                          idxNode.constDomain()->end(), [&](const Int val) {
                            return outputNode.constDomain()->contains(
                                parameters.at(val - offset));
                          });
    }
    if (!isFixed(idx)) {
      const auto& idxNode = _invariantGraph->varNode(idx);
      return std::none_of(idxNode.constDomain()->begin(),
                          idxNode.constDomain()->end(), [&](const Int val) {
                            return parameters.at(val - offset) ==
                                   boolVal(output);
                          });
    }
    if (!isFixed(output)) {
      return !_invariantGraph->varNodeConst(output).constDomain()->contains(
          parameters.at(intVal(idx) - offset));
    }
    return parameters.at(intVal(idx) - offset) != boolVal(output);
  }

  [[nodiscard]] bool alwaysSatisfied() const override {
    if (!isFixed(output)) {
      return false;
    }
    if (!isFixed(idx)) {
      const auto& idxNode = _invariantGraph->varNode(idx);
      return std::all_of(idxNode.constDomain()->begin(),
                         idxNode.constDomain()->end(), [&](const Int val) {
                           return parameters.at(val - offset) ==
                                  boolVal(output);
                         });
    }
    return parameters.at(intVal(idx) - offset) != boolVal(output);
  }

  [[nodiscard]] bool canMove() const override {
    return varId(idx) != propagation::NULL_ID;
  }

  void move(bool committedValue) override { changeValue(idx, committedValue); }

  void query() override {
    _solver->query(totalViolationVarId() != propagation::NULL_ID
                       ? totalViolationVarId()
                       : varId(output));
  }
};

RC_GTEST_FIXTURE_PROP(array_bool_elementTest, RapidCheck, ()) { rapidCheck(); }
}  // namespace atlantis::testing
