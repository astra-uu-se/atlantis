#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMinimumNodeTestFixture
    : public NodeTestBase<ArrayIntMinimumNode> {
 public:
  Int numInputs = 3;
  std::vector<Var> inputVars;
  Var outputVar{NULL_NODE_ID, "output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int val = std::numeric_limits<Int>::max();
      for (const auto& var : inputVars) {
        if (varNode(var).isFixed() || varId(var) == propagation::NULL_ID) {
          val = std::min(val, varNode(var).upperBound());
        } else {
          val = std::min(val, _solver->currentValue(varId(var)));
        }
      }
      return val;
    }
    Int val = std::numeric_limits<Int>::max();
    for (const auto& var : inputVars) {
      val = std::min(val, varNode(var).upperBound());
    }
    return val;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    std::vector<std::pair<Int, Int>> bounds;

    if (shouldBeSubsumed()) {
      bounds = {{-2, 5}, {-5, 2}, {-5, -5}};

    } else if (shouldBeReplaced()) {
      bounds = {{-5, 2}, {2, 5}, {2, 2}};
    } else {
      bounds = {{-5, 0}, {-2, -2}, {0, 5}};
    }
    for (const auto& [lb, ub] : bounds) {
      inputVars.emplace_back(
          makeIntVar(lb, ub, "input_" + std::to_string(inputVars.size())));
    }
    outputVar.id = retrieveIntVarNode(-5, 5, outputVar.identifier);

    createInvariantNode(*_invariantGraph, varNodeIds(inputVars), outputVar.id);
  }
};

TEST_P(ArrayIntMinimumNodeTestFixture, updateState) {
  Int minVal = std::numeric_limits<Int>::min();
  Int maxVal = std::numeric_limits<Int>::max();
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
        _invariantGraph->varNode(outputVarNodeId).upperBound();
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_LE(minVal, varNode(outputVar).lowerBound());
  EXPECT_GE(maxVal, varNode(outputVar).upperBound());
}

TEST_P(ArrayIntMinimumNodeTestFixture, replace) {
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

TEST_P(ArrayIntMinimumNodeTestFixture, propagation) {
  Int ub = std::numeric_limits<Int>::max();
  for (const auto& var : inputVars) {
    ub = std::min(ub, varNode(var).upperBound());
  }

  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(true);
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }
  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar).isFixed());
    EXPECT_EQ(varId(outputVar), propagation::NULL_ID);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (varNode(var).lowerBound() < ub) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  VarNode& outputNode = varNode(outputVar);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(true);
    EXPECT_EQ(expected, actual);
    return;
  }
  EXPECT_NE(varId(outputVar), propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputVar);

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
    ArrayIntMinimumNodeTest, ArrayIntMinimumNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
