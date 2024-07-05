#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolNotNodeTestFixture : public NodeTestBase<BoolNotNode> {
 public:
  VarNodeId inputVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string inputIdentifier{"input"};
  std::string outputIdentifier{"output"};

  bool computeOutput() { return varNode(inputVarNodeId).inDomain(bool{false}); }

  bool computeOutput(propagation::Solver& solver) {
    return solver.currentValue(varId(inputVarNodeId)) > 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeId = retrieveBoolVarNode(inputIdentifier);
    outputVarNodeId = retrieveBoolVarNode(outputIdentifier);

    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        varNode(inputVarNodeId).fixToValue(bool{true});
      } else {
        varNode(outputVarNodeId).fixToValue(bool{true});
      }
    }

    createInvariantNode(inputVarNodeId, outputVarNodeId);
  }
};

TEST_P(BoolNotNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), inputVarNodeId);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(BoolNotNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // inputVarNodeId
  EXPECT_EQ(solver.searchVars().size(), 1);

  // inputVarNodeId
  EXPECT_EQ(solver.numVars(), 1);
}

TEST_P(BoolNotNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
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

TEST_P(BoolNotNodeTestFixture, propagation) {
  if (shouldBeSubsumed()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  const propagation::VarId inputId = varId(inputIdentifier);
  EXPECT_NE(inputId, propagation::NULL_ID);

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  for (Int inputVal = solver.lowerBound(inputId);
       inputVal <= solver.upperBound(inputId); ++inputVal) {
    solver.beginMove();
    solver.setValue(inputId, inputVal);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const bool expected = computeOutput(solver);
    const bool actual = solver.currentValue(outputId) == 0;
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    BoolNotNodeTest, BoolNotNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::REPLACE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE, int{1}}));

}  // namespace atlantis::testing
