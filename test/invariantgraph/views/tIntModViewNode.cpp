#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModViewNodeTestFixture : public NodeTestBase<IntModViewNode> {
 public:
  VarNodeId inputVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string inputIdentifier{"input"};
  std::string outputIdentifier{"output"};

  Int denominator{5};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVarNodeId)) %
             std::abs(denominator);
    }
    return varNode(inputVarNodeId).domain().lowerBound() %
           std::abs(denominator);
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    const Int lb = shouldBeSubsumed() ? 5 : -10;
    const Int ub = shouldBeSubsumed() ? 5 : 10;
    inputVarNodeId = retrieveIntVarNode(lb, ub, inputIdentifier);
    outputVarNodeId = retrieveIntVarNode(0, 5, outputIdentifier);

    createInvariantNode(*_invariantGraph, inputVarNodeId, outputVarNodeId,
                        denominator);
  }
};

TEST_P(IntModViewNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), inputVarNodeId);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntModViewNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // inputVarNodeId
  EXPECT_EQ(_solver->searchVars().size(), 1);

  // inputVarNodeId
  EXPECT_EQ(_solver->numVars(), 1);
}

TEST_P(IntModViewNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVarNodeId).isFixed());
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).domain().lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntModViewNodeTestFixture, propagation) {
  if (shouldBeSubsumed()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply();
  _invariantGraph->close();

  const propagation::VarId inputId = varId(inputIdentifier);
  EXPECT_NE(inputId, propagation::NULL_ID);

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = _solver->lowerBound(inputId);
       inputVal <= _solver->upperBound(inputId); ++inputVal) {
    _solver->beginMove();
    _solver->setValue(inputId, inputVal);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals({inputId}, {inputVal});

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    IntModViewNodeTest, IntModViewNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
