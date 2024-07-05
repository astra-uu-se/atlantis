#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntDivNodeTestFixture : public NodeTestBase<IntDivNode> {
 public:
  VarNodeId numeratorVarNodeId{NULL_NODE_ID};
  VarNodeId denominatorVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int denominatorVal() { return varNode(denominatorVarNodeId).lowerBound(); }

  Int denominatorVal(propagation::Solver& solver) {
    return varNode(denominatorVarNodeId).isFixed()
               ? varNode(denominatorVarNodeId).lowerBound()
               : solver.currentValue(varId(denominatorVarNodeId));
  }

  Int computeOutput() {
    const Int numerator = varNode(numeratorVarNodeId).lowerBound();
    const Int denominator = denominatorVal();
    return denominator != 0 ? numerator / denominator : 0;
  }

  Int computeOutput(propagation::Solver& solver) {
    const Int numerator = varNode(numeratorVarNodeId).isFixed()
                              ? varNode(numeratorVarNodeId).lowerBound()
                              : solver.currentValue(varId(numeratorVarNodeId));
    const Int denominator = denominatorVal(solver);
    return denominator != 0 ? numerator / denominator : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numeratorVarNodeId = retrieveIntVarNode(-2, 2, "numeratorVarNodeId");
    if (shouldBeReplaced()) {
      denominatorVarNodeId = retrieveIntVarNode(1, 1, "denominatorVarNodeId");
    } else {
      denominatorVarNodeId = retrieveIntVarNode(-2, 2, "denominatorVarNodeId");
    }
    outputVarNodeId = retrieveIntVarNode(-2, 2, outputIdentifier);

    createInvariantNode(numeratorVarNodeId, denominatorVarNodeId,
                        outputVarNodeId);
  }
};

TEST_P(IntDivNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().numerator(), numeratorVarNodeId);
  EXPECT_EQ(invNode().denominator(), denominatorVarNodeId);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntDivNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // numeratorVarNodeId and denominatorVarNodeId
  EXPECT_EQ(solver.searchVars().size(), 2);

  // numeratorVarNodeId, denominatorVarNodeId and outputVarNodeId
  EXPECT_EQ(solver.numVars(), 3);

  // intDiv
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntDivNodeTestFixture, replace) {
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

TEST_P(IntDivNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(solver);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId :
       std::array<VarNodeId, 2>{numeratorVarNodeId, denominatorVarNodeId}) {
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

    if (denominatorVal(solver) != 0) {
      const Int actual = solver.currentValue(outputId);
      const Int expected = computeOutput(solver);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    IntDivNodeTest, IntDivNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
