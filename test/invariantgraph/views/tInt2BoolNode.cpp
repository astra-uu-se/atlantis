#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/int2BoolNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class Int2BoolNodeTestFixture : public NodeTestBase<Int2BoolNode> {
 public:
  VarNodeId inputVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string inputIdentifier{"input"};
  std::string outputIdentifier{"output"};

  bool computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      return _solver->currentValue(varId(inputIdentifier)) == 1;
    }
    return varNode(inputIdentifier).inDomain(Int{1});
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeId = retrieveIntVarNode(0, 1, inputIdentifier);
    outputVarNodeId = retrieveBoolVarNode(outputIdentifier);

    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        varNode(inputVarNodeId).fixToValue(Int{0});
      } else {
        varNode(outputVarNodeId).fixToValue(bool{true});
      }
    }

    createInvariantNode(*_invariantGraph, inputVarNodeId, outputVarNodeId);
  }
};

TEST_P(Int2BoolNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), inputVarNodeId);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(Int2BoolNodeTestFixture, application) {
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

TEST_P(Int2BoolNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(inputVarNodeId).isFixed());
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const bool expected = computeOutput();
    const bool actual = varNode(outputVarNodeId).inDomain(bool{true});
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(Int2BoolNodeTestFixture, propagation) {
  if (shouldBeSubsumed()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  const propagation::VarViewId inputId = varId(inputIdentifier);
  EXPECT_NE(inputId, propagation::NULL_ID);

  const propagation::VarViewId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = _solver->lowerBound(inputId);
       inputVal <= _solver->upperBound(inputId); ++inputVal) {
    _solver->beginMove();
    _solver->setValue(inputId, inputVal);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    const bool expected = computeOutput(true);
    const bool actual = _solver->currentValue(outputId) == 0;
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Int2BoolNodeTest, Int2BoolNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::REPLACE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE, int{1}}));

}  // namespace atlantis::testing
