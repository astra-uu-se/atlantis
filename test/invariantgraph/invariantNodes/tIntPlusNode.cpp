#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntPlusNodeTest : public NodeTestBase<IntPlusNode> {
 public:
  std::vector<VarNodeId> inputs;
  VarNodeId output{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs = std::vector<VarNodeId>{retrieveIntVarNode(0, 10, "a"),
                                    retrieveIntVarNode(0, 10, "b")};
    output = retrieveIntVarNode(0, 10, "output");

    createInvariantNode(inputs.front(), inputs.back(), output);
  }
};

TEST_F(IntPlusNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputs.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(IntPlusNodeTest, application) {
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

  // a and b
  EXPECT_EQ(solver.searchVars().size(), 2);

  // a, b and output
  EXPECT_EQ(solver.numVars(), 3);

  // intPow
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(IntPlusNodeTest, updateState) {
  const Int fixedValue = 5;
  const Int expected = static_cast<Int>(inputs.size()) * fixedValue;
  for (const auto& input : inputs) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    _invariantGraph->varNode(input).fixToValue(fixedValue);
    invNode().updateState(*_invariantGraph);
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  EXPECT_TRUE(_invariantGraph->varNode(output).isFixed());
  const Int actual = _invariantGraph->varNode(output).lowerBound();
  EXPECT_EQ(actual, expected);
}

TEST_F(IntPlusNodeTest, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  _invariantGraph->varNode(inputs.front()).fixToValue(Int{0});
  invNode().updateState(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(IntPlusNodeTest, propagation) {
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
    const Int expected = inputVals.at(0) + inputVals.at(1);
    EXPECT_EQ(actual, expected);
  }
}
}  // namespace atlantis::testing
