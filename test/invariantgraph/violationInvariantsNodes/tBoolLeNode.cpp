#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLeNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolLeNodeTestFixture : public NodeTestBase<BoolLeNode> {
 protected:
  Var aVar{"a", std::vector<Int>{}, false};
  Var bVar{"b", std::vector<Int>{}, false};
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  bool isViolating(const bool isRegistered = false) {
    if (isRegistered) {
      const bool aVal = varNode(aVar).isFixed()
                            ? varNode(aVar).inDomain(bool{true})
                            : _solver->currentValue(varId(aVar)) == 0;
      const bool bVal = varNode(bVar).isFixed()
                            ? varNode(bVar).inDomain(bool{true})
                            : _solver->currentValue(varId(bVar)) == 0;
      // !(a <= b) <=> a > b
      return aVal && !bVal;
    }
    const bool aVal = varNode(aVar).inDomain(bool{true});
    const bool bVal = varNode(bVar).inDomain(bool{true});
    // !(a <= b) <=> a > b
    return aVal && !bVal;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    reifiedVar.domain = std::pair<Int, Int>{0, 1};

    aVar.domain = std::pair<Int, Int>{0, 1};
    bVar.domain = std::pair<Int, Int>{0, 1};

    bVar.identifier = "b";

    if (shouldBeReplaced()) {
      if (isReified()) {
        if (_paramData.data == 0) {
          aVar.domain = std::vector<Int>{1};
        } else {
          bVar.domain = std::vector<Int>{0};
        }
      }
    }
    if (shouldBeSubsumed()) {
      if (isReified() || shouldHold()) {
        if (_paramData.data == 0) {
          aVar.domain = std::vector<Int>{0};
        } else {
          bVar.domain = std::vector<Int>{1};
        }
      }
    }
    if (shouldBeSubsumed() && _paramData.data == 2) {
      bVar.identifier = aVar.identifier;
      bVar.domain = aVar.domain;
    }

    retrieveBoolVarNode(aVar);
    retrieveBoolVarNode(bVar);

    if (isReified()) {
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeId(aVar), varNodeId(bVar),
                          varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeId(aVar), varNodeId(bVar),
                          shouldHold());
    }
  }
};

TEST_P(BoolLeNodeTestFixture, updateState) {
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
  }
}

TEST_P(BoolLeNodeTestFixture, replace) {
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

TEST_P(BoolLeNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeReplaced() && varId(reifiedVar) == propagation::NULL_ID) {
    EXPECT_TRUE(isReified());
    EXPECT_EQ(varId(reifiedVar), propagation::NULL_ID);
    EXPECT_FALSE(varNode(reifiedVar).isFixed());
    return;
  }

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
  for (const auto& var :
       std::array<VarNodeId, 2>{varNodeId(aVar), varNodeId(bVar)}) {
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

INSTANTIATE_TEST_SUITE_P(
    BoolLeNodeTest, BoolLeNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED, 0},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED, 1},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE, 2},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 0},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 1},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 2}));

}  // namespace atlantis::testing
