#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class IntCountNodeTestFixture : public NodeTestBase<IntCountNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};
  Int needle{2};

  Int computeOutput() {
    Int occurrences = 0;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      occurrences += varNode(inputVarNodeId).isFixed() &&
                             varNode(inputVarNodeId).inDomain(needle)
                         ? 1
                         : 0;
    }
    return occurrences;
  }

  Int computeOutput(propagation::Solver& solver) {
    Int occurrences = 0;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      if (varNode(inputVarNodeId).isFixed() ||
          varId(inputVarNodeId) == propagation::NULL_ID) {
        occurrences += varNode(inputVarNodeId).isFixed() &&
                               varNode(inputVarNodeId).inDomain(needle)
                           ? 1
                           : 0;
      } else {
        occurrences +=
            solver.currentValue(varId(inputVarNodeId)) == needle ? 1 : 0;
      }
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVarNodeIds = {
            retrieveIntVarNode(0, 1, "input1"),
            retrieveIntVarNode(std::vector<Int>{1, 3, 4, 5, 6, 7, 8, 9, 10},
                               "input2"),
            retrieveIntVarNode(std::vector<Int>{2}, "input3")};
        outputVarNodeId = retrieveIntVarNode(0, 3, outputIdentifier);
      } else {
        inputVarNodeIds = {retrieveIntVarNode(2, 2, "input1"),
                           retrieveIntVarNode(1, 10, "input2"),
                           retrieveIntVarNode(1, 10, "input3")};
        outputVarNodeId = retrieveIntVarNode(0, 1, outputIdentifier);
      }
    } else {
      inputVarNodeIds = {retrieveIntVarNode(1, 10, "input1"),
                         retrieveIntVarNode(1, 10, "input2"),
                         retrieveIntVarNode(1, 10, "input3")};
      outputVarNodeId = retrieveIntVarNode(0, 3, outputIdentifier);
    }

    createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds}, needle,
                        outputVarNodeId);
  }
};

TEST_P(IntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());

  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputVarNodeIds);
  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{outputVarNodeId};

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

  // inputVarNodeIds.at(0), inputVarNodeIds.at(1), and inputVarNodeIds.at(2)
  EXPECT_EQ(solver.searchVars().size(), 3);
  // inputVarNodeIds.at(0), inputVarNodeIds.at(1), inputVarNodeIds.at(2), and
  // the violation
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

    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntCountNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(solver);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
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

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

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
