#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/linLeImplicitNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/intLinLeNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntLinLeImplicitNodeTestFixture : public NodeTestBase<LinLeImplicitNode> {
 public:
  Int numVars = 4;
  std::vector<std::string> inputVars;

  std::vector<Int> coeffs;
  Int bound = -7;

  void SetUp() override {
    NodeTestBase::SetUp();
    for (Int i = 0; i < numVars; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
      retrieveIntVarNode(-10, 10, inputVars.back());
      coeffs.emplace_back((i % 2 == 0 ? 2 : -2) * (i + 1));
    }

    createImplicitConstraintNode(*_invariantGraph, std::vector<Int>{coeffs},
                                 varNodeIds(inputVars), bound);
  }
};

TEST_P(IntLinLeImplicitNodeTestFixture, construction) {
  EXPECT_EQ(invNode().outputVarNodeIds(), varNodeIds(inputVars));
}

TEST_P(IntLinLeImplicitNodeTestFixture, application) {
  _solver->open();
  _solverMapping = std::make_shared<SolverMapping>();
  invNode().registerOutputVars(*_solver, *_solverMapping);
  for (const VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_solver, *_solverMapping);
  _solver->close();

  // a, b, c and d
  EXPECT_EQ(_solver->searchVars().size(), 4);

  // a, b, c and d
  EXPECT_EQ(_solver->numVars(), 4);

  EXPECT_EQ(_solver->numInvariants(), 0);

  const auto neighborhood = _solverMapping->neighborhood(_invNodeId);

  EXPECT_TRUE(dynamic_cast<search::neighborhoods::IntLinLeNeighborhood*>(
      neighborhood.get()));
}

INSTANTIATE_TEST_SUITE_P(IntLinLeImplicitNodeTest,
                         IntLinLeImplicitNodeTestFixture,
                         ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
