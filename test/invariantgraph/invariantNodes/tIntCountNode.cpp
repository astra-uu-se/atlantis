#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/propagation/solver.hpp"

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
  std::vector<VarNodeId> inputs;
  VarNodeId output{NULL_NODE_ID};
  Int needle{2};

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs = {retrieveIntVarNode(1, 10, "input1"),
              retrieveIntVarNode(1, 10, "input2"),
              retrieveIntVarNode(1, 10, "input3")};

    output = retrieveIntVarNode(0, 2, "output");

    createInvariantNode(std::vector<VarNodeId>{inputs}, needle, output);
  }
};

TEST_F(IntCountNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputs.size());

  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputs);
  EXPECT_THAT(inputs, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{output};

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

  // inputs.at(0), inputs.at(1), and inputs.at(2)
  EXPECT_EQ(solver.searchVars().size(), 3);
  // inputs.at(0), inputs.at(1), inputs.at(2), and the violation
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
