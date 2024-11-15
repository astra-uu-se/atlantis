#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMaximumNodeTestFixture
    : public NodeTestBase<ArrayIntMaximumNode> {
 public:
  Int numInputs = 3;
  std::vector<Var> inputVars;
  Var outputVar{NULL_NODE_ID, "output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int val = std::numeric_limits<Int>::min();
      for (const auto& var : inputVars) {
        if (varNode(var).isFixed() || varId(var) == propagation::NULL_ID) {
          val = std::max(val, varNode(var).upperBound());
        } else {
          val = std::max(val, _solver->currentValue(varId(var)));
        }
      }
      return val;
    }
    Int val = std::numeric_limits<Int>::min();
    for (const auto& var : inputVars) {
      val = std::max(val, varNode(var.identifier).upperBound());
    }
    return val;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    std::vector<std::pair<Int, Int>> bounds;

    if (shouldBeSubsumed()) {
      bounds = {{-5, 2}, {-2, 5}, {5, 5}};
    } else if (shouldBeReplaced()) {
      bounds = {{-2, 5}, {-5, -2}, {-2, -2}};
    } else {
      bounds = {{0, 5}, {2, 2}, {-5, 0}};
    }
    for (const auto& [lb, ub] : bounds) {
      inputVars.emplace_back(
          makeIntVar(lb, ub, "input_" + std::to_string(inputVars.size())));
    }
    outputVar.id = retrieveIntVarNode(-5, 5, outputVar.identifier);

    createInvariantNode(*_invariantGraph, varNodeIds(inputVars), outputVar.id);
  }
};

TEST_P(ArrayIntMaximumNodeTestFixture, updateState) {
  Int minVal = std::numeric_limits<Int>::max();
  Int maxVal = std::numeric_limits<Int>::min();
  for (const auto& var : inputVars) {
    minVal = std::min(minVal, varNode(var).lowerBound());
    maxVal = std::max(maxVal, varNode(var).upperBound());
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    // EXPECT_TRUE(_invariantGraph->varNode(outputVarNodeId).isFixed());
    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual =
        _invariantGraph->varNode(outputVarNodeId).lowerBound();
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_LE(minVal, varNode(outputVar).lowerBound());
  EXPECT_GE(maxVal, varNode(outputVar).upperBound());
}

TEST_P(ArrayIntMaximumNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else if (invNode().state() == InvariantNodeState::ACTIVE) {
    EXPECT_FALSE(invNode().canBeReplaced());
    EXPECT_FALSE(invNode().replace());
  }
}

TEST_P(ArrayIntMaximumNodeTestFixture, propagation) {
  Int lb = std::numeric_limits<Int>::min();
  for (const auto& var : inputVars) {
    lb = std::max(lb, varNode(var).lowerBound());
  }

  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    [[maybe_unused]] const Int expected = computeOutput(true);
    [[maybe_unused]] const Int actual = varNode(outputVarNodeId).lowerBound();
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    /*
    const Int expected = computeOutput(true);
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
    */
    return;
  }
  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar.identifier).isFixed());
    EXPECT_EQ(varId(outputVar.identifier), propagation::NULL_ID);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (varNode(var).upperBound() > lb) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  VarNode& outputNode = varNode(outputVar.identifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(true);
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputVar.identifier), propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputVar.identifier);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayIntMaximumNodeTest, ArrayIntMaximumNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
