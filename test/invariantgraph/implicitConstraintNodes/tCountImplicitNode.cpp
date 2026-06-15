#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighborhoods/countNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class CountNodeTestFixture : public NodeTestBase<CountImplicitNode> {
 protected:
  Int numVars = 4;
  std::vector<std::string> inputVars;
  size_t amount = 2;
  Int needle = 0;

  void SetUp() override {
    NodeTestBase::SetUp();
    for (Int i = 0; i < numVars; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
      if (_paramData.data == 0) {
        retrieveIntVarNode(-10, 10, inputVars.back());
      } else {
        retrieveBoolVarNode(inputVars.back());
      }
    }

    createImplicitConstraintNode(*_invariantGraph, varNodeIds(inputVars),
                                 needle, amount);
  }
};

TEST_P(CountNodeTestFixture, construction) {
  EXPECT_EQ(invNode().outputVarNodeIds(), varNodeIds(inputVars));
}

TEST_P(CountNodeTestFixture, application) {
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

  EXPECT_TRUE(dynamic_cast<search::neighborhoods::CountNeighborhood*>(
      neighborhood.get()));
}

INSTANTIATE_TEST_SUITE_P(CountNodeTest, CountNodeTestFixture,
                         ::testing::Values(ParamData{0}, ParamData{1}));

}  // namespace atlantis::testing
