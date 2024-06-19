#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntTimesNodeTest : public NodeTestBase<IntTimesNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveIntVarNode(0, 10, "a");
    b = retrieveIntVarNode(0, 10, "b");
    output = retrieveIntVarNode(0, 10, "output");

    createInvariantNode(a, b, output);
  }
};

TEST_F(IntTimesNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().a(), a);
  EXPECT_EQ(invNode().b(), b);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(IntTimesNodeTest, application) {
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

  EXPECT_EQ(solver.lowerBound(varId(output)), 0);
  EXPECT_EQ(solver.upperBound(varId(output)), 100);

  // a and b
  EXPECT_EQ(solver.searchVars().size(), 2);

  // a, b and output
  EXPECT_EQ(solver.numVars(), 3);

  // intTimes
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(IntTimesNodeTest, updateState) {
  for (const auto& input : inputs) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    _invariantGraph->varNode(input).fixToValue(Int{0});
    invNode().updateState(*_invariantGraph);
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  EXPECT_TRUE(_invariantGraph->varNode(output).isFixed());
}

TEST_F(IntTimesNodeTest, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  for (Int i = 0; i < numInputs - 1; ++i) {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
    _invariantGraph->varNode(inputs.at(i)).fixToValue(Int{0});
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(IntTimesNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputVars;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputVars.size(), 2);

  solver.close();

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int expected = inputVals.at(0) * inputVals.at(1);
    EXPECT_EQ(actual, expected);
  }
}
}  // namespace atlantis::testing
