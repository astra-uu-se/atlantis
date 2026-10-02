#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/tableImplicitNode.hpp"
#include "atlantis/search/neighborhoods/tableNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class TableImplicitNodeTestFixture : public NodeTestBase<TableImplicitNode> {
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
    std::vector<std::vector<Int>> table(7 - 2 + 1,
                                        std::vector<Int>(vars.size()));
    for (size_t row = 0; row < table.size(); row++) {
      for (size_t col = 0; col < table.at(row).size(); col++) {
        table.at(row).at(col) = (static_cast<Int>(row + col) % 6) + 2;
      }
    }
    createImplicitConstraintNode(*_invariantGraph, std::move(vars),
                                 std::move(table));
  }
};

TEST_P(TableImplicitNodeTestFixture, construction) {
  const std::vector<std::shared_ptr<VarNode>> expectedVars{a, b, c, d};

  EXPECT_THAT(invNode().outputVarNodes(), ::testing::ContainerEq(expectedVars));
}

TEST_P(TableImplicitNodeTestFixture, application) {
  _solver->open();
  _solverMapping = std::make_shared<SolverMapping>();
  invNode().registerOutputVars(*_solver, *_solverMapping);
  for (const auto& outputVarNode : invNode().outputVarNodes()) {
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

  EXPECT_TRUE(dynamic_cast<search::neighborhoods::TableNeighborhood*>(
      neighborhood.get()));
}

INSTANTIATE_TEST_SUITE_P(TableImplicitNodeTest, TableImplicitNodeTestFixture,
                         ::testing::Values(ParamData{}));

}  // namespace atlantis::testing
