#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntDivNodeTestFixture : public NodeTestBase<IntDivNode> {
 public:
  VarNodeId numeratorVarNodeId{NULL_NODE_ID};
  std::string numeratorIdentifier{"numerator"};
  VarNodeId denominatorVarNodeId{NULL_NODE_ID};
  std::string denominatorIdentifier{"denominator"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int denominatorVal(bool isRegistered = false) {
    if (isRegistered) {
      return varNode(denominatorIdentifier).isFixed()
                 ? varNode(denominatorIdentifier).lowerBound()
                 : _solver->currentValue(varId(denominatorIdentifier));
    }
    return varNode(denominatorVarNodeId).lowerBound();
  }

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int numerator =
          varNode(numeratorIdentifier).isFixed()
              ? varNode(numeratorIdentifier).lowerBound()
              : _solver->currentValue(varId(numeratorIdentifier));
      const Int denominator = denominatorVal(true);
      return denominator != 0 ? numerator / denominator : 0;
    }
    const Int numerator = varNode(numeratorIdentifier).lowerBound();
    const Int denominator = denominatorVal();
    return denominator != 0 ? numerator / denominator : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numeratorVarNodeId = retrieveIntVarNode(-2, 2, numeratorIdentifier);
    if (shouldBeReplaced()) {
      denominatorVarNodeId = retrieveIntVarNode(1, 1, denominatorIdentifier);
    } else {
      denominatorVarNodeId = retrieveIntVarNode(-2, 2, denominatorIdentifier);
    }
    outputVarNodeId = retrieveIntVarNode(-2, 2, outputIdentifier);

    createInvariantNode(*_invariantGraph, numeratorVarNodeId,
                        denominatorVarNodeId, outputVarNodeId);
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
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  EXPECT_EQ(_solver->searchVars().size(), 2);

  EXPECT_EQ(_solver->numVars(), 3);

  // intDiv
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(IntDivNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced());
  }
}

TEST_P(IntDivNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeReplaced()) {
    EXPECT_EQ(varNode(outputIdentifier).varNodeId(),
              varNode(numeratorIdentifier).varNodeId());
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier :
       std::array<std::string, 2>{numeratorIdentifier, denominatorIdentifier}) {
    if (!varNode(identifier).isFixed()) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }

  const propagation::VarViewId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    if (denominatorVal(true) != 0) {
      const Int actual = _solver->currentValue(outputId);
      const Int expected = computeOutput(true);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    IntDivNodeTest, IntDivNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
