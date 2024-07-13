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
  std::vector<std::string> inputIdentifiers{"input_1", "input_2", "input_3"};
  VarNodeId needleVarNodeId{NULL_NODE_ID};
  std::string needleIdentifier{"needle"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput() {
    const Int needleVal = varNode(needleIdentifier).lowerBound();
    Int occurrences = 0;
    for (const auto& identifier : inputIdentifiers) {
      EXPECT_TRUE(varNode(identifier).isFixed() ||
                  !varNode(identifier).inDomain(needleVal));

      occurrences += varNode(identifier).isFixed() &&
                             varNode(identifier).inDomain(needleVal)
                         ? 1
                         : 0;
    }
    return occurrences;
  }

  Int computeOutput(propagation::Solver& solver) {
    const Int needleVal = varNode(needleIdentifier).isFixed()
                              ? varNode(needleIdentifier).lowerBound()
                              : solver.currentValue(varId(needleIdentifier));
    Int occurrences = 0;
    for (const auto& identifier : inputIdentifiers) {
      const VarNode& inputVarNode = varNode(identifier);
      if (!inputVarNode.inDomain(needleVal)) {
        continue;
      }
      if (inputVarNode.isFixed() || varId(identifier) == propagation::NULL_ID) {
        EXPECT_TRUE(inputVarNode.isFixed());
        EXPECT_TRUE(varNode(identifier).inDomain(needleVal));
        ++occurrences;
      } else {
        occurrences +=
            solver.currentValue(varId(identifier)) == needleVal ? 1 : 0;
      }
    }
    return occurrences;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds = {retrieveIntVarNode(2, 5, inputIdentifiers.at(0)),
                       retrieveIntVarNode(3, 5, inputIdentifiers.at(1)),
                       retrieveIntVarNode(4, 5, inputIdentifiers.at(2))};
    if (shouldBeReplaced()) {
      needleVarNodeId = retrieveIntVarNode(2, 2, needleIdentifier);
    } else {
      needleVarNodeId = retrieveIntVarNode(2, 5, needleIdentifier);
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
  _invariantGraph->close(solver);

  if (shouldBeReplaced()) {
    EXPECT_TRUE(varNode(needleIdentifier).isFixed());
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (varNode(identifier).isFixed()) {
      continue;
    }
    if (varNode(needleIdentifier).isFixed()) {
      const Int needleVar = varNode(needleIdentifier).lowerBound();
      if (!varNode(identifier).inDomain(needleVar)) {
        continue;
      }
    }
    EXPECT_NE(varId(identifier), propagation::NULL_ID);
    inputVarIds.emplace_back(varId(identifier));
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
    VarIntCountNodeTest, VarIntCountNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
