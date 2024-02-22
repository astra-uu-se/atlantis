#include "../nodeTestBase.hpp"
#include "invariantgraph/fzn/circuitImplicitNode.hpp"
#include "invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"
#include "propagation/solver.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class CircuitImplicitNodeTest : public NodeTestBase<CircuitImplicitNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId c{NULL_NODE_ID};
  VarNodeId d{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = defineIntVarNode(1, 4, "a");
    b = defineIntVarNode(1, 4, "b");
    c = defineIntVarNode(1, 4, "c");
    d = defineIntVarNode(1, 4, "d");

    std::vector<VarNodeId> vars{a, b, c, d};

    createImplicitConstraintNode(std::move(vars));
  }
};

TEST_F(CircuitImplicitNodeTest, construction) {
  std::vector<VarNodeId> expectedVars{a, b, c, d};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedVars);
}

TEST_F(CircuitImplicitNodeTest, application) {
  propagation::Solver solver;
  solver.open();
  for (VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // a, b, c and d
  EXPECT_EQ(solver.searchVars().size(), 4);

  // a, b, c and d
  EXPECT_EQ(solver.numVars(), 4);

  EXPECT_EQ(solver.numInvariants(), 0);

  auto neighbourhood = invNode().neighbourhood();

  EXPECT_TRUE(dynamic_cast<search::neighbourhoods::CircuitNeighbourhood*>(
      neighbourhood.get()));
}

}  // namespace atlantis::testing