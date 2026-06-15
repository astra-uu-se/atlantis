#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolOrNodeTestFixture : public NodeTestBase<ArrayBoolOrNode> {
 protected:
  Int numInputVars{4};
  std::vector<Var> inputVars;
  Var reifiedVar{"reified", std::vector<Int>{}, false};

  [[nodiscard]] bool isViolating(const bool isRegistered = false) const {
    if (isRegistered) {
      return std::ranges::all_of(inputVars, [&](const auto& var) {
        if (varNodeConst(var).isFixed()) {
          return !varNodeConst(var).inDomain(bool{true});
        }
        return _solver->currentValue(varId(var)) != 0;
      });
    }
    return std::ranges::all_of(inputVars, [&](const auto& var) {
      return !varNodeConst(var).inDomain(bool{true});
    });
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVars.clear();
    inputVars.reserve(numInputVars);
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(Var::BoolVar("input_" + std::to_string(i)));
    }

    if (shouldBeSubsumed()) {
      if (isReified()) {
        for (auto& var : inputVars) {
          var.fixToValue(false);
        }
      } else if (shouldHold()) {
        inputVars.front().fixToValue(true);
      }
    } else if (shouldBeReplaced()) {
      for (size_t i = 1; i < inputVars.size(); ++i) {
        inputVars.at(i).fixToValue(false);
      }
    }

    for (const auto& var : inputVars) {
      retrieveBoolVarNode(var);
    }

    if (isReified()) {
      reifiedVar.domain = std::vector<Int>{0, 1};
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          shouldHold());
    }
  }
};

TEST_P(ArrayBoolOrNodeTestFixture, updateState) {
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

TEST_P(ArrayBoolOrNodeTestFixture, replace) {
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

TEST_P(ArrayBoolOrNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  if (shouldBeSubsumed()) {
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
  if (shouldBeReplaced()) {
    EXPECT_TRUE(isReified());
    EXPECT_FALSE(varNode(reifiedVar).isFixed());
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
    ArrayBoolOrNodeTest, ArrayBoolOrNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
