#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/intLinEqImplicitNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/intLinEqNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntLinEqImplicitNodeTestFixture
    : public NodeTestBase<IntLinEqImplicitNode> {
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
    }

    createImplicitConstraintNode(*_invariantGraph, std::vector<Int>{coeffs},
                                 varNodeIds(inputVars), bound);
  }
};

TEST_P(IntLinEqImplicitNodeTestFixture, construction) {
  EXPECT_EQ(invNode().outputVarNodeIds(), varNodeIds(inputVars));
}

TEST_P(IntLinEqImplicitNodeTestFixture, application) {
  _solver->open();
  for (const VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const VarNodeId outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // a, b, c and d
  EXPECT_EQ(_solver->searchVars().size(), 4);

  // a, b, c and d
  EXPECT_EQ(_solver->numVars(), 4);

  EXPECT_EQ(_solver->numInvariants(), 0);

  const auto neighborhood = invNode().neighborhood();

  EXPECT_TRUE(dynamic_cast<search::neighborhoods::IntLinEqNeighborhood*>(
      neighborhood.get()));
}

INSTANTIATE_TEST_SUITE_P(IntLinEqImplicitNodeTest,
                         IntLinEqImplicitNodeTestFixture,
                         ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
