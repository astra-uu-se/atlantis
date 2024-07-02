#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class VarIntCountNodeTestFixture : public NodeTestBase<VarIntCountNode> {
 public:
  std::vector<VarNodeId> inputs;
  VarNodeId needle{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  std::string outputIdentifier{"output"};

  Int computeOutput(propagation::Solver& solver) {
    const Int needleVal = varNode(needle).isFixed()
                              ? varNode(needle).lowerBound()
                              : solver.currentValue(varId(needle));
    Int occurrences = 0;
    for (const auto& input : inputs) {
      if (varNode(input).isFixed() || varId(input) == propagation::NULL_ID) {
        occurrences +=
            varNode(input).isFixed() && varNode(input).inDomain(needleVal) ? 1
                                                                           : 0;
      } else {
        occurrences += solver.currentValue(varId(input)) == needleVal ? 1 : 0;
      }
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs = {retrieveIntVarNode(2, 5, "input_1"),
              retrieveIntVarNode(3, 5, "input_2"),
              retrieveIntVarNode(4, 5, "input_3")};
    if (shouldBeReplaced()) {
      needle = retrieveIntVarNode(2, 2, "needle");
    } else {
      needle = retrieveIntVarNode(2, 5, "needle");
    }

    output = retrieveIntVarNode(0, 2, "output");

    createInvariantNode(std::vector<VarNodeId>{inputs}, needle, output);
  }
};

TEST_P(VarIntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs{inputs};
  expectedInputs.emplace_back(needle);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{output};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(VarIntCountNodeTestFixture, application) {
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

  // x1, x2, x3, and needleVar
  EXPECT_EQ(solver.searchVars().size(), 4);
  // x1, x2, x3, needleVar, and (output or intermediate)
  EXPECT_EQ(solver.numVars(), 5);

  // countEq
  EXPECT_EQ(solver.numInvariants(), 1);

  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(solver.lowerBound(varId(outputVarNodeId)), 0);
    EXPECT_GT(solver.upperBound(varId(outputVarNodeId)), 0);
  }
}

TEST_P(VarIntCountNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(VarIntCountNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(VarIntCountNodeTest, VarIntCountNodeTestFixture,
                        ::testing::Values(ParamData{InvariantNodeAction::NONE},
                                          ParamData{
                                              InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
