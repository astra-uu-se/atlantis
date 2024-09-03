#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/bool2IntNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class Bool2IntNodeTestFixture : public NodeTestBase<Bool2IntNode> {
 public:
  VarNodeId inputVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string inputIdentifier{"input"};
  std::string outputIdentifier{"output"};

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return _solver->currentValue(varId(inputVarNodeId)) == 0 ? 1 : 0;
    }
    return varNode(inputVarNodeId).inDomain(bool{true}) ? 1 : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeId = retrieveBoolVarNode(inputIdentifier);
    outputVarNodeId = retrieveIntVarNode(0, 1, outputIdentifier);

    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        varNode(inputVarNodeId).fixToValue(bool{true});
      } else {
        varNode(outputVarNodeId).fixToValue(Int{0});
      }
    }

    createInvariantNode(*_invariantGraph, inputVarNodeId, outputVarNodeId);
  }
};

TEST_P(Bool2IntNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), inputVarNodeId);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(Bool2IntNodeTestFixture, application) {
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

TEST_P(Bool2IntNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVarNodeId).isFixed());
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(Bool2IntNodeTestFixture, propagation) {
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

    const Int expected = computeOutput(true);
    const Int actual = _solver->currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Bool2IntNodeTest, Bool2IntNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::REPLACE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE, int{1}}));

}  // namespace atlantis::testing
