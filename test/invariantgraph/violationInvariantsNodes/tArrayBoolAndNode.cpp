#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolAndNodeTestFixture : public NodeTestBase<ArrayBoolAndNode> {
 protected:
  Int numInputs{4};
  std::vector<Var> inputVars;
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      for (const auto& var : inputVars) {
        if (varNodeConst(var).isFixed()) {
          if (varNodeConst(var).inDomain(bool{false})) {
            return true;
          }
        } else {
          EXPECT_NE(varId(var), propagation::NULL_ID);
          if (_solver->currentValue(varId(var)) > 0) {
            return true;
          }
        }
      }
      return false;
    }
    for (const auto& var : inputVars) {
      if (varNodeConst(var).inDomain(bool{false})) {
        return true;
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVars.clear();
    inputVars.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputVars.emplace_back(Var::BoolVar("input_" + std::to_string(i)));
    }

    if (shouldBeSubsumed()) {
      if (isReified()) {
        for (auto& var : inputVars) {
          var.fixToValue(true);
        }
      } else if (!shouldHold()) {
        inputVars.front().fixToValue(false);
      }
    } else if (shouldBeReplaced()) {
      for (size_t i = 1; i < inputVars.size(); ++i) {
        inputVars.at(i).fixToValue(!shouldFail());
      }
    }

    for (const auto& var : inputVars) {
      retrieveBoolVarNode(var);
    }

    if (isReified()) {
      reifiedVar.domain = std::vector<Int>{0, 1};
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          varNodeId(reifiedVar.identifier));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          shouldHold());
    }
  }
};

TEST_P(ArrayBoolAndNodeTestFixture, updateState) {
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

TEST_P(ArrayBoolAndNodeTestFixture, replace) {
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

TEST_P(ArrayBoolAndNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeReplaced() && isReified()) {
    EXPECT_EQ(varId(reifiedVar), propagation::NULL_ID);
    EXPECT_FALSE(varNode(reifiedVar).isFixed());
    return;
  }

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _solverMapping->totalViolationId();

  if (shouldBeSubsumed()) {
    EXPECT_TRUE(shouldBeSubsumed() || shouldBeReplaced());
    const bool expected = isViolating(true);
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
      EXPECT_NE(varId(var), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(var));
    }
  }

  EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());

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
    ArrayBoolAndNodeTest, ArrayBoolAndNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
