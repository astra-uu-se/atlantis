#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMaximumNodeTestFixture
    : public NodeTestBase<ArrayIntMaximumNode> {
 protected:
  std::vector<Var> inputVars;
  Var outputVar{"output", std::vector<Int>{}, true};

  Int computeOutput(const bool isRegistered = false) {
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
      val = std::max(val, varNode(var).upperBound());
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
      inputVars.emplace_back("input_" + std::to_string(inputVars.size()), lb, ub, true);
      retrieveIntVarNode(inputVars.back());
    }
    outputVar.domain = std::pair<Int, Int>(-5, 5);
    retrieveIntVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                        varNodeId(outputVar));
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
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    // TODO: disabled for the MZN challenge. This should be computed by Gecode.
    // EXPECT_TRUE(_invariantGraph->varNode(outputVarNodeId).isFixed());
    [[maybe_unused]] const Int expected = computeOutput();
    [[maybe_unused]] const Int actual = varNode(outputVar).lowerBound();
    // TODO: disabled for the MZN challenge. This should be computed by Gecode.
    // EXPECT_EQ(expected, actual);
  } else {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_LE(minVal, varNode(outputVar).lowerBound());
  EXPECT_GE(maxVal, varNode(outputVar).upperBound());
}

TEST_P(ArrayIntMaximumNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
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

  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    [[maybe_unused]] const Int expected = computeOutput(true);
    [[maybe_unused]] const Int actual = varNode(outputVar).lowerBound();
    // TODO: disabled for the MZN challenge. This should be computed by Gecode.
    /*
    const Int expected = computeOutput(true);
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
    */
    return;
  }
  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar).isFixed());
    EXPECT_EQ(varId(outputVar), propagation::NULL_ID);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (varNode(var).upperBound() > lb) {
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

INSTANTIATE_TEST_SUITE_P(
    ArrayIntMaximumNodeTest, ArrayIntMaximumNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
