#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolClauseNodeTestFixture : public NodeTestBase<BoolClauseNode> {
 protected:
  std::vector<Var> posVars;
  std::vector<Var> negVars;
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  Int numPos{2};
  Int numNeg{2};

  bool isViolating(const bool isRegistered = false) {
    if (isRegistered) {
      for (const auto& a : posVars) {
        for (const auto& b : negVars) {
          if (varNodeId(a) == varNodeId(b)) {
            return false;
          }
        }
      }
      for (const auto& a : posVars) {
        if (varNode(a).isFixed()) {
          if (varNode(a).inDomain(bool{true})) {
            return false;
          }
        } else if (_solver->currentValue(varId(a)) == 0) {
          return false;
        }
      }
      for (const auto& b : negVars) {
        if (varNode(b).isFixed()) {
          if (varNode(b).inDomain(bool{false})) {
            return false;
          }
        } else if (_solver->currentValue(varId(b)) > 0) {
          return false;
        }
      }
      return true;
    }
    for (const auto& a : posVars) {
      for (const auto& b : negVars) {
        if (varNode(a).varNodeId() == varNode(b).varNodeId()) {
          return false;
        }
      }
    }
    for (const auto& a : posVars) {
      const auto& vNode = varNode(a);
      if (vNode.inDomain(bool{true})) {
        return false;
      }
    }
    for (const auto& b : negVars) {
      const auto& vNode = varNode(b);
      if (vNode.inDomain(bool{false})) {
        return false;
      }
    }
    return true;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    posVars.clear();
    negVars.clear();
    numPos = 2;
    numNeg = 2;
    if (shouldBeReplaced()) {
      if (_paramData.data == 0) {
        numPos = 0;
      } else if (_paramData.data == 1) {
        numNeg = 0;
      }
    }

    posVars.reserve(numPos);
    negVars.reserve(numNeg);

    for (Int i = 0; i < numPos; ++i) {
      std::vector<Int> dom{0, 1};
      if (shouldBeSubsumed() && (isReified() || shouldHold()) &&
          (_paramData.data != 0 || i != 0)) {
        dom = {shouldFail() ? 0 : 1};
      }
      posVars.emplace_back("a_" + std::to_string(i), std::move(dom), false);
      retrieveBoolVarNode(posVars.back());
    }
    for (Int i = 0; i < numNeg; ++i) {
      std::vector<Int> dom{0, 1};
      if (shouldBeSubsumed() && (isReified() || shouldHold()) &&
          (_paramData.data != 1 || i != 0)) {
        dom = {shouldFail() ? 1 : 0};
      }
      negVars.emplace_back("b_" + std::to_string(i), std::move(dom), false);
      retrieveBoolVarNode(negVars.back());
    }

    if (isReified()) {
      reifiedVar.domain = std::pair<Int, Int>{0, 1};
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(posVars),
                          varNodeIds(negVars), varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(posVars),
                          varNodeIds(negVars), shouldHold());
    }
  }
};

TEST_P(BoolClauseNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedVar).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVar).isFixed());
    }
  }
}

TEST_P(BoolClauseNodeTestFixture, replace) {
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

TEST_P(BoolClauseNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain(false);
      EXPECT_EQ(expected, actual);
    }
    if (shouldHold()) {
      EXPECT_FALSE(expected);
    }
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& var : posVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }
  for (const auto& var : negVars) {
    if (!varNode(var).isFixed()) {
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  EXPECT_EQ(inputVarIds.size() <= 1, shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    EXPECT_EQ(isViolating(true), shouldFail());
    return;
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

INSTANTIATE_TEST_SUITE_P(
    BoolClauseNodeTest, BoolClauseNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, int{0}},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, int{1}},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, int{2}},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE, int{0}},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE, int{1}},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE, int{2}},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE, int{1}},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
