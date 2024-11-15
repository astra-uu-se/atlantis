#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntDivNodeTestFixture : public NodeTestBase<IntDivNode> {
 public:
  Var numeratorVar{NULL_NODE_ID, "numerator"};
  Var denominatorVar{NULL_NODE_ID, "denominator"};
  Var outputVar{NULL_NODE_ID, "output"};

  Int denominatorVal(bool isRegistered = false) {
    if (isRegistered) {
      return varNode(denominatorVar).isFixed()
                 ? varNode(denominatorVar).lowerBound()
                 : _solver->currentValue(varId(denominatorVar));
    }
    return varNode(denominatorVar).lowerBound();
  }

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int numerator = varNode(numeratorVar).isFixed()
                                ? varNode(numeratorVar).lowerBound()
                                : _solver->currentValue(varId(numeratorVar));
      const Int denominator = denominatorVal(true);
      return denominator != 0 ? numerator / denominator : 0;
    }
    const Int numerator = varNode(numeratorVar).lowerBound();
    const Int denominator = denominatorVal();
    return denominator != 0 ? numerator / denominator : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numeratorVar.id = retrieveIntVarNode(-2, 2, numeratorVar.identifier);
    if (shouldBeReplaced()) {
      denominatorVar.id = retrieveIntVarNode(1, 1, denominatorVar.identifier);
    } else {
      denominatorVar.id = retrieveIntVarNode(-2, 2, denominatorVar.identifier);
    }
    outputVar.id = retrieveIntVarNode(-2, 2, outputVar.identifier);

    createInvariantNode(*_invariantGraph, numeratorVar.id, denominatorVar.id,
                        outputVar.id);
  }
};

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
    EXPECT_EQ(varNode(outputVar).varNodeId(),
              varNode(numeratorVar).varNodeId());
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : std::array<Var, 2>{numeratorVar, denominatorVar}) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId outputId = varId(outputVar);
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
