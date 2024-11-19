#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolXorNodeTestFixture : public NodeTestBase<ArrayBoolXorNode> {
 public:
  std::vector<std::string> inputVars;

  std::string reifiedVar{"reified"};

  Int numInputs = 4;

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      bool trueFound = false;
      for (const auto& var : inputVars) {
        if (varNode(var).isFixed()) {
          if (varNode(var).inDomain(bool{true})) {
            if (trueFound) {
              return true;
            }
            trueFound = true;
          }
        } else {
          if (_solver->currentValue(varId(var)) == 0) {
            if (trueFound) {
              return true;
            }
            trueFound = true;
          }
        }
      }
      return !trueFound;
    }
    bool trueFound = false;
    for (const auto& var : inputVars) {
      if (varNode(var).inDomain(bool{true})) {
        if (trueFound) {
          return true;
        }
        trueFound = true;
      }
    }
    return !trueFound;
  }

  void generate() {
    inputVars.clear();
    inputVars.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputVars.emplace_back("input_" + std::to_string(i));
      retrieveBoolVarNode(inputVars.back());
    }

    if (shouldBeSubsumed() || shouldBeReplaced()) {
      if (shouldBeSubsumed()) {
        varNode(inputVars.front()).fixToValue(shouldHold());
      }
      for (size_t i = 1; i < inputVars.size(); ++i) {
        varNode(inputVars.at(i)).fixToValue(shouldFail());
      }
    }

    if (isReified()) {
      retrieveBoolVarNode(reifiedVar);
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          varNodeId(reifiedVar));
    } else {
      createInvariantNode(*_invariantGraph, varNodeIds(inputVars),
                          shouldHold());
    }
  }
};

TEST_P(ArrayBoolXorNodeTestFixture, updateState) {
  generate();
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
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

TEST_P(ArrayBoolXorNodeTestFixture, replace) {
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

TEST_P(ArrayBoolXorNodeTestFixture, propagation) {
  generate();
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

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

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVar).isFixed());
      const bool actual = varNode(reifiedVar).inDomain({false});
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

  const propagation::VarViewId violVarId =
      isReified() ? varId(reifiedVar) : _invariantGraph->totalViolationVarId();

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
    ArrayBoolXorNodeTest, ArrayBoolXorNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
