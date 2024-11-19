#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntDivNodeTestFixture : public NodeTestBase<IntDivNode> {
 public:
  std::string numeratorVar{"numerator"};
  std::string denominatorVar{"denominator"};
  std::string outputVar{"output"};

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

  void generate() {
    retrieveIntVarNode(-2, 2, numeratorVar);
    if (shouldBeReplaced()) {
      retrieveIntVarNode(1, 1, denominatorVar);
    } else {
      retrieveIntVarNode(-2, 2, denominatorVar);
    }
    retrieveIntVarNode(-2, 2, outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(numeratorVar),
                        varNodeId(denominatorVar), varNodeId(outputVar));
  }
};

TEST_P(IntDivNodeTestFixture, replace) {
  generate();
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
  generate();
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeReplaced()) {
    EXPECT_EQ(varNode(outputVar).varNodeId(),
              varNode(numeratorVar).varNodeId());
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var :
       std::array<std::string, 2>{numeratorVar, denominatorVar}) {
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
