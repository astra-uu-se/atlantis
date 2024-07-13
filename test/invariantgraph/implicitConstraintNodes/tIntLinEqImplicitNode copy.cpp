#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/neighbourhoods/intLinEqNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntLinEqImplicitNodeTestFixture
    : public NodeTestBase<IntLinEqImplicitNode> {
 public:
  Int numVars = 4;
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers;
  std::vector<Int> coeffs;
  Int bound = -7;

  void SetUp() override {
    NodeTestBase::SetUp();
    for (Int i = 0; i < numVars; ++i) {
      inputIdentifiers.emplace_back("input_" + std::to_string(i));
      inputVarNodeIds.emplace_back(
          retrieveIntVarNode(-10, 10, inputIdentifiers.back()));
      coeffs.emplace_back(i % 2 == 0 ? 1 : -1);
    }

    createImplicitConstraintNode(std::vector<Int>{coeffs},
                                 std::vector<VarNodeId>{inputVarNodeIds},
                                 bound);
  }
};

TEST_P(IntLinEqImplicitNodeTestFixture, construction) {
  EXPECT_EQ(invNode().outputVarNodeIds(), inputVarNodeIds);
}

TEST_P(IntLinEqImplicitNodeTestFixture, application) {
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

  EXPECT_TRUE(dynamic_cast<search::neighbourhoods::IntLinEqNeighbourhood*>(
      neighbourhood.get()));
}

INSTANTIATE_TEST_SUITE_P(IntLinEqImplicitNodeTest,
                         IntLinEqImplicitNodeTestFixture,
                         ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
