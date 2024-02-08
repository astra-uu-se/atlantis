#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/intCountNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static Int computeOutput(const std::vector<Int>& values, const Int needle) {
  Int occurrences = 0;
  for (const Int val : values) {
    occurrences += (val == needle ? 1 : 0);
  }
  return occurrences;
}

class IntCountNodeTest : public NodeTestBase<IntCountNode> {
 public:
  VarNodeId x1{NULL_NODE_ID};
  VarNodeId x2{NULL_NODE_ID};
  VarNodeId x3{NULL_NODE_ID};
  VarNodeId count{NULL_NODE_ID};
  Int needle{2};

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVarNode(2, 5, "x1");
    x2 = createIntVarNode(3, 5, "x2");
    x3 = createIntVarNode(4, 5, "x3");
    std::vector<VarNodeId> inputVec{x1, x2, x3};

    count = createIntVarNode(0, 2, "count");

    createInvariantNode(std::move(inputVec), needle, count);
  }
};

TEST_F(IntCountNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs{x1, x2, x3};
  size_t inputSize = 3;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputSize);

  EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedInputs);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{count};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_F(IntCountNodeTest, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // x1, x2, and x3
  EXPECT_EQ(solver.searchVars().size(), 3);
  // x1, x2, x3, and the violation
  EXPECT_EQ(solver.numVars(), 4);

  // countEq
  EXPECT_EQ(solver.numInvariants(), 1);

  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(solver.lowerBound(varId(outputVarNodeId)), 0);
    EXPECT_GT(solver.upperBound(varId(outputVarNodeId)), 0);
  }
}

TEST_F(IntCountNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputVars;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }
  EXPECT_EQ(inputVars.size(), 3);

  EXPECT_EQ(invNode().needle(), needle);
  EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);

  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());

  solver.close();

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(inputVals, needle);

    EXPECT_EQ(actual, expected);
  }
}

}  // namespace atlantis::testing