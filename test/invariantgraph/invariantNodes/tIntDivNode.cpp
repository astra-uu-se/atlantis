#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntDivNodeTestFixture : public NodeTestBase<IntDivNode> {
 protected:
  Var numeratorVar{"numerator", std::vector<Int>{}, true};
  Var denominatorVar{"denominator", std::vector<Int>{}, true};
  Var outputVar{"output", std::vector<Int>{}, true};

  [[nodiscard]] Int denominatorVal(const bool isRegistered = false) const {
    if (isRegistered) {
      return varNodeConst(denominatorVar).isFixed()
                 ? varNodeConst(denominatorVar).lowerBound()
                 : _solver->currentValue(varId(denominatorVar));
    }
    return varNodeConst(denominatorVar).lowerBound();
  }

  Int computeOutput(const bool isRegistered = false) {
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
    numeratorVar.domain = std::pair<Int, Int>{-2, 2};
    if (shouldBeReplaced()) {
      denominatorVar.domain = std::pair<Int, Int>{1, 1};
    } else {
      denominatorVar.domain = std::pair<Int, Int>{-2, 2};
    }
    outputVar.domain = std::pair<Int, Int>{-2, 2};
    retrieveIntVarNode(numeratorVar);
    retrieveIntVarNode(denominatorVar);
    retrieveIntVarNode(outputVar);

    createInvariantNode(*_invariantGraph, varNodeId(numeratorVar),
                        varNodeId(denominatorVar), varNodeId(outputVar));
  }
};

TEST_P(IntDivNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
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
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeReplaced()) {
    EXPECT_EQ(varNode(outputVar).varNodeId(),
              varNode(numeratorVar).varNodeId());
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto var : std::array<VarNodeId, 2>{varNodeId(numeratorVar),
                                                 varNodeId(denominatorVar)}) {
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

INSTANTIATE_TEST_SUITE_P(
    IntDivNodeTest, IntDivNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
