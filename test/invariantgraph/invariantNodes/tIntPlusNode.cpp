#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;
using ::testing::Contains;

class IntPlusNodeTestFixture : public NodeTestBase<IntPlusNode> {
 public:
  std::vector<std::string> inputVars;
  std::string outputVar{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      Int sum = 0;
      for (const auto& var : inputVars) {
        if (varNode(var).isFixed()) {
          sum += varNode(var).lowerBound();
        } else {
          sum += _solver->currentValue(varId(var));
        }
      }
      return sum;
    }
    Int sum = 0;
    for (const auto& var : inputVars) {
      EXPECT_TRUE(varNode(var).isFixed());
      sum += varNode(var).lowerBound();
    }
    return sum;
  }

  void SetUp() {
    NodeTestBase::SetUp();
    for (size_t i = 0; i < 2; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
    }
    if (shouldBeSubsumed()) {
      retrieveIntVarNode(1, 1, inputVars.at(0));
      retrieveIntVarNode(1, 1, inputVars.at(1));
    } else if (shouldBeReplaced()) {
      retrieveIntVarNode(0, 0, inputVars.at(0));
      retrieveIntVarNode(-2, 2, inputVars.at(1));
    } else {
      retrieveIntVarNode(-2, 2, inputVars.at(0));
      retrieveIntVarNode(-2, 2, inputVars.at(1));
    }
    retrieveIntVarNode(0, 10, outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(inputVars.at(0)),
                        varNodeId(inputVars.at(1)), varNodeId(outputVar));
  }
};

TEST_P(IntPlusNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  const auto expectedInputs = varNodeIds(inputVars);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), varNodeId(outputVar));
}

TEST_P(IntPlusNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntPlusNodeTestFixture, replace) {
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

TEST_P(IntPlusNodeTestFixture, propagation) {
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const VarNode& outputNode = varNode(outputVar);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput(true);
    EXPECT_EQ(actual, expected);
    return;
  }

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar).isFixed());
    const VarNodeId addantVarNodeId =
        varNode(varNode(inputVars.front()).isFixed() ? inputVars.back()
                                                     : inputVars.front())
            .varNodeId();
    EXPECT_EQ(varNode(outputVar).varNodeId(), addantVarNodeId);
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
    IntPlusNodeTest, IntPlusNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
