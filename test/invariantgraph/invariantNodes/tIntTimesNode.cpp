#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntTimesNodeTestFixture : public NodeTestBase<IntTimesNode> {
 public:
  std::vector<std::string> inputVars;
  std::string outputVar{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int product = 1;
      for (const auto& var : inputVars) {
        if (varNode(var).isFixed()) {
          product *= varNode(var).lowerBound();
        } else {
          product *= _solver->currentValue(varId(var));
        }
      }
      return product;
    }
    Int product = 1;
    for (const auto& var : inputVars) {
      product *= varNode(var).lowerBound();
    }
    return product;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    for (size_t i = 0; i < 2; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
    }
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        retrieveIntVarNode(0, 0, inputVars.at(0));
        retrieveIntVarNode(0, 10, inputVars.at(1));
      } else {
        retrieveIntVarNode(-2, -2, inputVars.at(0));
        retrieveIntVarNode(2, 2, inputVars.at(1));
      }
    } else if (shouldBeReplaced()) {
      retrieveIntVarNode(1, 1, inputVars.at(0));
      retrieveIntVarNode(-2, 2, inputVars.at(1));
    } else {
      retrieveIntVarNode(-2, 2, inputVars.at(0));
      retrieveIntVarNode(-2, 2, inputVars.at(1));
    }
    retrieveIntVarNode(-10, 10, outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(inputVars.at(0)),
                        varNodeId(inputVars.at(1)), varNodeId(outputVar));
  }
};

TEST_P(IntTimesNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    // TODO: disabled for the MZN challenge. This should be computed by Gecode.
    /*
    EXPECT_TRUE(varNode(outputVar).isFixed());
    Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    TODO: disabled for the MZN challenge. This should be computed by Gecode.
    EXPECT_EQ(expected, actual);
    */
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntTimesNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced());
  }
}

TEST_P(IntTimesNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping = std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    // TODO: disabled for the MZN challenge. This should be computed by Gecode.
    /*
    VarNode& outputNode = varNode(outputVar);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput();
    EXPECT_EQ(actual, expected);
    */
    return;
  }

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar).isFixed());
    const VarNodeId factorVarNodeId =
        varNode(varNode(inputVars.front()).isFixed() ? inputVars.back()
                                                     : inputVars.front())
            .varNodeId();
    EXPECT_EQ(varNode(outputVar).varNodeId(), factorVarNodeId);
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputId);
    const Int expected = computeOutput(true);
    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    IntTimesNodeTest, IntTimesNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
