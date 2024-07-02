#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class IntCountNodeTestFixture : public NodeTestBase<IntCountNode> {
 public:
  std::vector<VarNodeId> inputs;
  VarNodeId output{NULL_NODE_ID};
  std::string outputIdentifier{"output"};
  Int needle{2};

  Int computeOutput(propagation::Solver& solver) {
    Int occurrences = 0;
    for (const auto& input : inputs) {
      if (varNode(input).isFixed() || varId(input) == propagation::NULL_ID) {
        occurrences +=
            varNode(input).isFixed() && varNode(input).inDomain(needle) ? 1 : 0;
      } else {
        occurrences += solver.currentValue(varId(input)) == needle ? 1 : 0;
      }
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputs = {retrieveIntVarNode(0, 1, "input1"),
                  retrieveIntVarNode(
                      std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10}, "input2"),
                  retrieveIntVarNode(std::vector<Int>{2}, "input3")};
        output = retrieveIntVarNode(0, 3, outputIdentifier);
      } else {
        inputs = {retrieveIntVarNode(2, 2, "input1"),
                  retrieveIntVarNode(1, 10, "input2"),
                  retrieveIntVarNode(1, 10, "input3")};
        output = retrieveIntVarNode(0, 1, outputIdentifier);
      }
    } else {
      inputs = {retrieveIntVarNode(1, 10, "input1"),
                retrieveIntVarNode(1, 10, "input2"),
                retrieveIntVarNode(1, 10, "input3")};
      output = retrieveIntVarNode(0, 3, outputIdentifier);
    }

    createInvariantNode(std::vector<VarNodeId>{inputs}, needle, output);
  }
};

TEST_P(IntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputs.size());

  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputs);
  EXPECT_THAT(inputs, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{output};

  EXPECT_EQ(invNode().outputVarNodeIds(), expectedOutputs);
  EXPECT_THAT(expectedOutputs, ContainerEq(invNode().outputVarNodeIds()));
}

TEST_P(IntCountNodeTestFixture, application) {
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

TEST_P(IntCountNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);

    Int expected = 0;
    for (const auto& input : inputs) {
      expected +=
          varNode(input).isFixed() && varNode(input).inDomain(needle) ? 1 : 0;
    }
    const Int actual = varNode(output).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(output).isFixed());
  }
}

TEST_P(IntCountNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  VarNode& outputNode = varNode(outputIdentifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(solver);
    EXPECT_EQ(expected, actual);
    return;
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

INSTANTIATE_TEST_CASE_P(
    IntCountNodeTest, IntCountNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE},
                      ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1}));

}  // namespace atlantis::testing
