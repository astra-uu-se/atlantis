#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class AllDifferentImplicitNodeTestFixture
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

    createImplicitConstraintNode(*_invariantGraph, std::move(vars));
  }
};

TEST_P(AllDifferentImplicitNodeTestFixture, construction) {
  std::vector<VarNodeId> expectedVars{a, b, c, d};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedVars);
}

TEST_P(AllDifferentImplicitNodeTestFixture, application) {
  _solver->open();
  for (VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // a, b, c and d
  EXPECT_EQ(_solver->searchVars().size(), 4);

  // a, b, c and d
  EXPECT_EQ(_solver->numVars(), 4);

  EXPECT_EQ(_solver->numInvariants(), 0);

  auto neighbourhood = invNode().neighbourhood();

  EXPECT_TRUE(
      dynamic_cast<search::neighbourhoods::AllDifferentUniformNeighbourhood*>(
          neighbourhood.get()));
}

INSTANTIATE_TEST_SUITE_P(AllDifferentImplicitNodeTest,
                         AllDifferentImplicitNodeTestFixture,
                         ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
