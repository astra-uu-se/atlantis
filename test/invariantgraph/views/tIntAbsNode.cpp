#include "../nodeTestBase.hpp"
#include "invariantgraph/views/intAbsNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntAbsNodeTest : public NodeTestBase<IntAbsNode> {
 public:
  VarNodeId input{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    input = defineIntVarNode(5, 10, "input");
    output = defineIntVarNode(2, 7, "output");

    createInvariantNode(input, output);
  }
};

TEST_F(IntAbsNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), input);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(IntAbsNodeTest, application) {
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
}  // namespace atlantis::testing