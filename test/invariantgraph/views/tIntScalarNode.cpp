#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntScalarNodeTest : public NodeTestBase<IntScalarNode> {
 public:
  VarNodeId input{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};
  Int factor{2};
  Int offset{5};

  void SetUp() override {
    NodeTestBase::SetUp();
    const Int lb = -10;
    const Int ub = 10;
    input = retrieveIntVarNode(lb, ub, "input");
    output = retrieveIntVarNode(lb * factor + offset, ub * factor + offset,
                                "output");

    createInvariantNode(input, output, factor, offset);
  }
};

TEST_F(IntScalarNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), input);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(IntScalarNodeTest, application) {
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

  // input
  EXPECT_EQ(solver.searchVars().size(), 1);

  // input
  EXPECT_EQ(solver.numVars(), 1);
}

TEST_F(IntScalarNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()),
            propagation::NULL_ID);
  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);

  const propagation::VarId inputVar =
      varId(invNode().staticInputVarNodeIds().front());
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());

  for (Int inputVal = solver.lowerBound(inputVar);
       inputVal <= solver.upperBound(inputVar); ++inputVal) {
    solver.beginMove();
    solver.setValue(inputVar, inputVal);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int expected = inputVal * factor + offset;
    const Int actual = solver.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

}  // namespace atlantis::testing
