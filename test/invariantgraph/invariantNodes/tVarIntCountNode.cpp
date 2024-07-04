#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class VarIntCountNodeTestFixture : public NodeTestBase<VarIntCountNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  VarNodeId needleVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};

  std::string outputIdentifier{"output"};

  Int computeOutput() {
    const Int needleVal = varNode(needleVarNodeId).lowerBound();
    Int occurrences = 0;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      EXPECT_TRUE(varNode(inputVarNodeId).isFixed() ||
                  !varNode(inputVarNodeId).inDomain(needleVal));

      occurrences += varNode(inputVarNodeId).isFixed() &&
                             varNode(inputVarNodeId).inDomain(needleVal)
                         ? 1
                         : 0;
    }
    return occurrences;
  }

  Int computeOutput(propagation::Solver& solver) {
    const Int needleVal = varNode(needleVarNodeId).isFixed()
                              ? varNode(needleVarNodeId).lowerBound()
                              : solver.currentValue(varId(needleVarNodeId));
    Int occurrences = 0;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      if (varNode(inputVarNodeId).isFixed() ||
          varId(inputVarNodeId) == propagation::NULL_ID) {
        occurrences += varNode(inputVarNodeId).isFixed() &&
                               varNode(inputVarNodeId).inDomain(needleVal)
                           ? 1
                           : 0;
      } else {
        occurrences +=
            solver.currentValue(varId(inputVarNodeId)) == needleVal ? 1 : 0;
      }
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds = {retrieveIntVarNode(2, 5, "input_1"),
                       retrieveIntVarNode(3, 5, "input_2"),
                       retrieveIntVarNode(4, 5, "input_3")};
    if (shouldBeReplaced()) {
      needleVarNodeId = retrieveIntVarNode(2, 2, "needle");
    } else {
      needleVarNodeId = retrieveIntVarNode(2, 5, "needle");
    }

    outputVarNodeId = retrieveIntVarNode(0, 2, outputIdentifier);

    createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                        needleVarNodeId, outputVarNodeId);
  }
};

TEST_P(VarIntCountNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  std::vector<VarNodeId> expectedInputs{inputVarNodeIds};
  expectedInputs.emplace_back(needleVarNodeId);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  std::vector<VarNodeId> expectedOutputs{outputVarNodeId};

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
  // x1, x2, x3, needleVar, and (outputVarNodeId or intermediate)
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

INSTANTIATE_TEST_CASE_P(VarIntCountNodeTest, VarIntCountNodeTestFixture,
                        ::testing::Values(ParamData{InvariantNodeAction::NONE},
                                          ParamData{
                                              InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
