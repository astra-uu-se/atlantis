#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLeNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntLeNodeTestFixture : public NodeTestBase<IntLeNode> {
 protected:
  Var aVar{"a", std::vector<Int>{}, false};
  Var bVar{"b", std::vector<Int>{}, false};
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      const Int aVal = varNodeConst(aVar).isFixed()
                           ? varNodeConst(aVar).lowerBound()
                           : _solver->currentValue(varId(aVar));
      const Int bVal = varNodeConst(bVar).isFixed()
                           ? varNodeConst(bVar).lowerBound()
                           : _solver->currentValue(varId(bVar));

      return aVal > bVal;
    }
    return varNodeConst(aVar).lowerBound() > varNodeConst(bVar).lowerBound();
  }

  void SetUp() {
    NodeTestBase::SetUp();
    aVar = retrieveIntVarNode(-5, 5, "a");
    bVar = retrieveIntVarNode(-5, 5, "b");
    if (shouldBeSubsumed()) {
      if (shouldHold() || _paramData.data > 0) {
        // varNode(aVarNodeId).removeValuesAbove(0);
        // varNode(bVarNodeId).removeValuesBelow(0);
      } else {
        // varNode(aVarNodeId).removeValuesBelow(1);
        // varNode(bVarNodeId).removeValuesAbove(0);
      }
    }
    if (isReified()) {
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, aVar, bVar, varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, aVar, bVar, shouldHold());
    }
  }
};

TEST_P(IntLeNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    // TODO: disabled for the MZN challenge. This should be computed by Gecode.
    /*
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedVar).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
    */
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
  }
}

TEST_P(IntLeNodeTestFixture, replace) {
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

TEST_P(IntLeNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    // TODO: disabled for the MZN challenge. This should be computed by Gecode
    /*
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
      EXPECT_EQ(expected, actual);
    }
    */
    if (shouldHold()) {
      EXPECT_FALSE(expected);
    }
    // TODO: disabled for the MZN challenge. This should be computed by Gecode
    /*
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    */
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : std::array<VarNodeId, 2>{aVar, bVar}) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(violVarId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const bool actual = _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    IntLeNodeTest, IntLeNodeTestFixture,
    ::testing::Values(ParamData{},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 0},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 1},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
