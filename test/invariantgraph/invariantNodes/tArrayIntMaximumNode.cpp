#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_int_maximum.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMaximumNodeTest : public NodeTestBase<ArrayIntMaximumNode> {
 public:
  std::vector<VarNodeId> inputs;
  VarNodeId output{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs = {retrieveIntVarNode(-5, 2, "x1"), retrieveIntVarNode(-3, 3, "x2"),
              retrieveIntVarNode(-2, 5, "x3")};

    output = retrieveIntVarNode(-5, 5, "output");

    createInvariantNode(std::vector<VarNodeId>{inputs}, output);
  }
};

TEST_F(ArrayIntMaximumNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputs.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(ArrayIntMaximumNodeTest, application) {
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

  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    lb = std::max(lb, solver.lowerBound(varId(inputVarNodeId)));
    ub = std::max(ub, solver.upperBound(varId(inputVarNodeId)));
  }

  EXPECT_EQ(solver.lowerBound(varId(output)), lb);
  EXPECT_EQ(solver.upperBound(varId(output)), ub);

  // x1, x2, and x3
  EXPECT_EQ(solver.searchVars().size(), 3);

  // x1, x2 and output
  EXPECT_EQ(solver.numVars(), 4);

  // maxSparse
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayIntMaximumNodeTest, updateState) {
  for (const auto& input : inputs) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    _invariantGraph->varNode(input).fixToValue(
        _invariantGraph->varNode(input).lowerBound());
    invNode().updateState(*_invariantGraph);
    Int minVal = std::numeric_limits<Int>::max();
    Int maxVal = std::numeric_limits<Int>::min();
    for (const auto& var : inputs) {
      minVal = std::min(minVal, _invariantGraph->varNode(var).lowerBound());
      maxVal = std::max(maxVal, _invariantGraph->varNode(var).upperBound());
    }
    EXPECT_LE(minVal, _invariantGraph->varNode(output).lowerBound());
    EXPECT_GE(maxVal, _invariantGraph->varNode(output).upperBound());
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  EXPECT_TRUE(_invariantGraph->varNode(output).isFixed());
}

TEST_F(ArrayIntMaximumNodeTest, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  for (size_t i = 0; i < inputs.size() - 1; ++i) {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
    _invariantGraph->varNode(inputs.at(i))
        .fixToValue(_invariantGraph->varNode(inputs.at(i)).lowerBound());
    invNode().updateState(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(ArrayIntMaximumNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputVars;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputVars.size(), 3);

  solver.close();
  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int expected = *std::max_element(inputVals.begin(), inputVals.end());
    const Int actual = solver.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

}  // namespace atlantis::testing
