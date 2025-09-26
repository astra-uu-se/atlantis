#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/intAbsNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntAbsNodeTestFixture : public NodeTestBase<IntAbsNode> {
 public:
  std::string outputVar{"output"};
  std::string inputVar{"input"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return std::abs(_solver->currentValue(varId(inputVar)));
    }
    return std::abs(varNode(inputVar).domain()->lowerBound());
  }

  void SetUp() {
    NodeTestBase::SetUp();
    retrieveIntVarNode(-10, 10, inputVar);
    retrieveIntVarNode(0, 10, outputVar);

    if (shouldBeSubsumed()) {
      varNode(inputVar).fixToValue(Int{-5});
    } else if (shouldBeReplaced()) {
      varNode(inputVar).domain()->removeBelow(0);
    }

    createInvariantNode(*_invariantGraph, varNodeId(inputVar),
                        varNodeId(outputVar));
  }
};

TEST_P(IntAbsNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVar).isFixed());
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).domain()->lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
  }
}

TEST_P(IntAbsNodeTestFixture, replace) {
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

TEST_P(IntAbsNodeTestFixture, propagation) {
  if (shouldBeSubsumed()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputVar).isFixed());
    EXPECT_FALSE(varNode(inputVar).isFixed());
    EXPECT_EQ(varNode(outputVar).varNodeId(), varNode(inputVar).varNodeId());
    return;
  }

  const propagation::VarViewId inputId = varId(inputVar);
  EXPECT_NE(inputId, propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = _solver->lowerBound(inputId);
       inputVal <= _solver->upperBound(inputId); ++inputVal) {
    _solver->beginMove();
    _solver->setValue(inputId, inputVal);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    IntAbsNodeTestTest, IntAbsNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
