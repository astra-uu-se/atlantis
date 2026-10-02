#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class AllDifferentImplicitNodeTestFixture
    : public NodeTestBase<AllDifferentImplicitNode> {
 protected:
  std::shared_ptr<VarNode> a{nullptr};
  std::shared_ptr<VarNode> b{nullptr};
  std::shared_ptr<VarNode> c{nullptr};
  std::shared_ptr<VarNode> d{nullptr};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveIntVarNode(2, 7, "a");
    b = retrieveIntVarNode(2, 7, "b");
    c = retrieveIntVarNode(2, 7, "c");
    d = retrieveIntVarNode(2, 7, "d");

    std::vector<std::shared_ptr<VarNode>> vars{a, b, c, d};

    createImplicitConstraintNode(*_invariantGraph, std::move(vars));
  }
};

TEST_P(AllDifferentImplicitNodeTestFixture, construction) {
  const std::vector<std::shared_ptr<VarNode>> expectedVars{a, b, c, d};

  EXPECT_EQ(invNode().outputVarNodes(), expectedVars);
}

TEST_P(AllDifferentImplicitNodeTestFixture, application) {
  _solver->open();
  _solverMapping = std::make_shared<SolverMapping>();
  invNode().registerOutputVars(*_solver, *_solverMapping);
  for (const auto& outputVarNod : invNode().outputVarNodes()) {
    EXPECT_NE(varId(outputVarNod), propagation::NULL_ID);
  }
  invNode().registerNode(*_solver, *_solverMapping);
  _solver->close();

  // a, b, c and d
  EXPECT_EQ(_solver->searchVars().size(), 4);

  // a, b, c and d
  EXPECT_EQ(_solver->numVars(), 4);

  EXPECT_EQ(_solver->numInvariants(), 0);

  const auto neighborhood = _solverMapping->neighborhood(_invNodeId);

  EXPECT_TRUE(
      dynamic_cast<search::neighborhoods::AllDifferentUniformNeighborhood*>(
          neighborhood.get()));
}

INSTANTIATE_TEST_SUITE_P(AllDifferentImplicitNodeTest,
                         AllDifferentImplicitNodeTestFixture,
                         ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
