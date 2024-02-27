#include "../nodeTestBase.hpp"
#include "invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "propagation/solver.hpp"
#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class AllDifferentImplicitNodeTest
    : public NodeTestBase<AllDifferentImplicitNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId c{NULL_NODE_ID};
  VarNodeId d{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveIntVarNode(2, 7, "a");
    b = retrieveIntVarNode(2, 7, "b");
    c = retrieveIntVarNode(2, 7, "c");
    d = retrieveIntVarNode(2, 7, "d");

    std::vector<VarNodeId> vars{a, b, c, d};

    createImplicitConstraintNode(std::move(vars));
  }
};

TEST_F(AllDifferentImplicitNodeTest, construction) {
  std::vector<VarNodeId> expectedVars{a, b, c, d};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedVars);
}

TEST_F(AllDifferentImplicitNodeTest, application) {
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

  EXPECT_TRUE(
      dynamic_cast<search::neighbourhoods::AllDifferentUniformNeighbourhood*>(
          neighbourhood.get()));
}

}  // namespace atlantis::testing