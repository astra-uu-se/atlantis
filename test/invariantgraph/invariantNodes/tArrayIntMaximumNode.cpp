#include "../nodeTestBase.hpp"
#include "invariantgraph/fzn/array_int_maximum.hpp"
#include "invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMaximumTestNode : public NodeTestBase<ArrayIntMaximumNode> {
 public:
  VarNodeId x1 = NULL_NODE_ID;
  VarNodeId x2 = NULL_NODE_ID;
  VarNodeId x3 = NULL_NODE_ID;
  VarNodeId output = NULL_NODE_ID;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVarNode(-5, 0, "x1");
    x2 = createIntVarNode(-2, 2, "x2");
    x3 = createIntVarNode(0, 5, "x3");
    output = createIntVarNode(0, 10, "output", true);

    std::vector<VarNodeId> inputs{x1, x2, x3};

    createInvariantNode(std::move(inputs), output);
  }
};

TEST_F(ArrayIntMaximumTestNode, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), x1);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(1), x2);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(2), x3);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(ArrayIntMaximumTestNode, application) {
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

TEST_F(ArrayIntMaximumTestNode, propagation) {
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