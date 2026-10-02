#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"
#include "atlantis/search/neighborhoods/circuitNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class CircuitImplicitNodeTestFixture
    : public NodeTestBase<CircuitImplicitNode> {
 protected:
  std::shared_ptr<VarNode> a{nullptr};
  std::shared_ptr<VarNode> b{nullptr};
  std::shared_ptr<VarNode> c{nullptr};
  std::shared_ptr<VarNode> d{nullptr};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = retrieveIntVarNode(1, 4, "a");
    b = retrieveIntVarNode(1, 4, "b");
    c = retrieveIntVarNode(1, 4, "c");
    d = retrieveIntVarNode(1, 4, "d");

    std::vector<std::shared_ptr<VarNode>> vars{a, b, c, d};

    createImplicitConstraintNode(*_invariantGraph, std::move(vars), 1);
  }
};

TEST_P(CircuitImplicitNodeTestFixture, construction) {
  const std::vector<std::shared_ptr<VarNode>> expectedVars{a, b, c, d};

  EXPECT_EQ(invNode().outputVarNodes(), expectedVars);
}

TEST_P(CircuitImplicitNodeTestFixture, application) {
  _solver->open();
  _solverMapping = std::make_shared<SolverMapping>();
  invNode().registerOutputVars(*_solver, *_solverMapping);
  for (const VarNod& outputVarNode : invNode().outputVarNodes()) {
    EXPECT_NE(varId(outputVarNode), propagation::NULL_ID);
  }
  invNode().registerNode(*_solver, *_solverMapping);
  _solver->close();

  // a, b, c and d
  EXPECT_EQ(_solver->searchVars().size(), 4);

  // a, b, c and d
  EXPECT_EQ(_solver->numVars(), 4);

  EXPECT_EQ(_solver->numInvariants(), 0);

  const auto neighborhood = _solverMapping->neighborhood(_invNodeId);

  EXPECT_TRUE(dynamic_cast<search::neighborhoods::CircuitNeighborhood*>(
      neighborhood.get()));
}

INSTANTIATE_TEST_CASE_P(CircuitImplicitNodeTest, CircuitImplicitNodeTestFixture,
                        ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
