#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantGraphRoot.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class BoolAllEqualNodeTestFixture : public NodeTestBase<BoolAllEqualNode> {
 protected:
  Int numInputs{4};
  std::vector<Var> inputVars;
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      bool allSameVarNodeId = true;
      for (size_t i = 0; i < inputVars.size(); ++i) {
        for (size_t j = i + 1; j < inputVars.size(); ++j) {
          if (varNodeConst(inputVars.at(i)).varNodeId() !=
              varNodeConst(inputVars.at(j)).varNodeId()) {
            allSameVarNodeId = false;
            break;
          }
        }
        if (!allSameVarNodeId) {
          break;
        }
      }
      if (allSameVarNodeId) {
        return false;
      }
      for (size_t i = 0; i < inputVars.size(); ++i) {
        const bool iVal =
            varNodeConst(inputVars.at(i)).isFixed()
                ? varNodeConst(inputVars.at(i)).inDomain(bool{true})
                : _solver->currentValue(varId(inputVars.at(i))) == 0;
        for (size_t j = i + 1; j < inputVars.size(); ++j) {
          const bool jVal =
              varNodeConst(inputVars.at(j)).isFixed()
                  ? varNodeConst(inputVars.at(j)).inDomain(bool{true})
                  : _solver->currentValue(varId(inputVars.at(j))) == 0;
          if (iVal != jVal) {
            return true;
          }
        }
      }
      return false;
    }
    bool allSameVarNodeId = true;
    for (size_t i = 0; i < inputVars.size(); ++i) {
      for (size_t j = i + 1; j < inputVars.size(); ++j) {
        if (varNodeConst(inputVars.at(i)).varNodeId() !=
            varNodeConst(inputVars.at(j)).varNodeId()) {
          allSameVarNodeId = false;
          break;
        }
      }
      if (!allSameVarNodeId) {
        break;
      }
    }
    if (allSameVarNodeId) {
      return false;
    }
    for (size_t i = 0; i < inputVars.size(); ++i) {
      for (size_t j = i + 1; j < inputVars.size(); ++j) {
        if (varNodeConst(inputVars.at(i)).inDomain(bool{true}) !=
            varNodeConst(inputVars.at(j)).inDomain(bool{true})) {
          return true;
        }
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numInputs = !shouldBeReplaced() || shouldHold() ? 4 : 2;

    for (Int i = 0; i < numInputs; ++i) {
      std::vector<Int> dom;
      if (!shouldBeSubsumed()) {
        dom = {0, 1};
      } else {
        dom.emplace_back(shouldHold() || i == 0 ? 1 : 0);
      }
      inputVars.emplace_back("input_" + std::to_string(i), std::move(dom),
                             false);
      retrieveBoolVarNode(inputVars.back());
    }
    if (!shouldBeMadeImplicit()) {
      for (const auto& var : inputVars) {
        _invariantGraph->root().addSearchVarNode(varNodeId(var));
      }
    }
    if (isReified()) {
      reifiedVar.domain = std::pair<Int, Int>{0, 1};
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          varNodeId(reifiedVar), !shouldBeReplaced());
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars), shouldHold(),
                          !shouldBeReplaced());
    }
  }
};

TEST_P(BoolAllEqualNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  const auto expectedInputs = varNodeIds(inputVars);
  EXPECT_THAT(expectedInputs, ContainerEq(invNode().staticInputVarNodeIds()));

  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), varNodeId(reifiedVar));
  }
}

TEST_P(BoolAllEqualNodeTestFixture, application) {
  _solver->open();
  _solverMapping = std::make_shared<SolverMapping>();
  addInputVarsToSolver();
  EXPECT_EQ(invNode().violationVarId(*_solverMapping), propagation::NULL_ID);
  invNode().registerOutputVars(*_solver, *_solverMapping);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_NE(invNode().violationVarId(*_solverMapping), propagation::NULL_ID);
  invNode().registerNode(*_solver, *_solverMapping);
  _solver->close();

  for (const auto& identifier : inputVars) {
    EXPECT_TRUE(varId(identifier).isVar());
    EXPECT_THAT(_solver->searchVars(),
                ::testing::Contains(size_t(varId(identifier))));
  }

  EXPECT_GE(_solver->numVars(),
            size_t(invNode().violationVarId(*_solverMapping)));

  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(BoolAllEqualNodeTestFixture, updateState) {
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

TEST_P(BoolAllEqualNodeTestFixture, replace) {
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

TEST_P(BoolAllEqualNodeTestFixture, propagation) {
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
  for (const auto& var : inputVars) {
    if (!varNode(var).isFixed()) {
      const propagation::VarViewId inputVarId = varId(var);
      EXPECT_NE(inputVarId, propagation::NULL_ID);
      const bool inVec = std::ranges::any_of(
          inputVarIds, [&](const propagation::VarViewId& varId) {
            return varId == inputVarId;
          });
      if (!inVec) {
        inputVarIds.emplace_back(inputVarId);
      }
    }
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  if (violVarId == propagation::NULL_ID) {
    EXPECT_EQ(inputVarIds.size(), 1);
    return;
  }

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    if (violVarId == propagation::NULL_ID) {
      EXPECT_EQ(inputVarIds.size(), 1);
      _solver->query(inputVarIds.front());
    } else {
      _solver->query(violVarId);
    }
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const bool actual = violVarId == propagation::NULL_ID
                            ? true
                            : _solver->currentValue(violVarId) > 0;
    const bool expected = isViolating(true);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    BoolAllEqualNodeTest, BoolAllEqualNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
