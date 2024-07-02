#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/views/intModViewNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModViewNodeTestFixture : public NodeTestBase<IntModViewNode> {
 public:
  VarNodeId input{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};
  Int denominator{5};

  void SetUp() override {
    NodeTestBase::SetUp();
    const Int lb = -10;
    const Int ub = 10;
    input = retrieveIntVarNode(lb, ub, "input");
    output = retrieveIntVarNode(0, 5, "output");

    createInvariantNode(input, output, denominator);
  }
};

TEST_P(IntModViewNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), input);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_P(IntModViewNodeTestFixture, application) {
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

TEST_P(IntModViewNodeTestFixture, propagation) {
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

    const Int expected = inputVal % std::abs(denominator);
    const Int actual = solver.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_SUITE_P(IntModViewNodeTest, IntModViewNodeTestFixture,
                         ::testing::Values(ParamData{
                             InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
