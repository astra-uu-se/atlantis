#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolXorNodeTestFixture : public NodeTestBase<ArrayBoolXorNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};
  Int numInputs = 4;

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      bool trueFound = false;
      for (const auto& identifier : inputIdentifiers) {
        if (varNode(identifier).isFixed()) {
          if (varNode(identifier).inDomain(bool{true})) {
            if (trueFound) {
              return true;
            }
            trueFound = true;
          }
        } else {
          if (_solver->currentValue(varId(identifier)) == 0) {
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
    for (const auto& identifier : inputIdentifiers) {
      if (varNode(identifier).inDomain(bool{true})) {
        if (trueFound) {
          return true;
        }
        trueFound = true;
      }
    }
    return !trueFound;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.clear();
    inputVarNodeIds.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputIdentifiers.emplace_back("input_" + std::to_string(i));
      inputVarNodeIds.emplace_back(
          retrieveBoolVarNode(inputIdentifiers.back()));
    }

    if (shouldBeSubsumed() || shouldBeReplaced()) {
      if (shouldBeSubsumed()) {
        varNode(inputVarNodeIds.front()).fixToValue(shouldHold());
      }
      for (size_t i = 1; i < inputVarNodeIds.size(); ++i) {
        varNode(inputVarNodeIds.at(i)).fixToValue(shouldFail());
      }
    }

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(*_invariantGraph,
                          std::vector<VarNodeId>{inputVarNodeIds},
                          reifiedVarNodeId);
    } else if (shouldHold()) {
      createInvariantNode(*_invariantGraph,
                          std::vector<VarNodeId>{inputVarNodeIds},
                          reifiedVarNodeId);
    } else {
      createInvariantNode(*_invariantGraph,
                          std::vector<VarNodeId>{inputVarNodeIds}, false);
    }
  }
};

TEST_P(ArrayBoolXorNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
  EXPECT_EQ(invNode().staticInputVarNodeIds(), inputVarNodeIds);
  EXPECT_THAT(inputVarNodeIds, ContainerEq(invNode().staticInputVarNodeIds()));
  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  }
}

TEST_P(ArrayBoolXorNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  EXPECT_EQ(_solver->searchVars().size(), numInputs);

  EXPECT_EQ(_solver->numVars(), numInputs + 1);

  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(ArrayBoolXorNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVarNodeId).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedVarNodeId).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedVarNodeId).isFixed());
    }
  }
}

TEST_P(ArrayBoolXorNodeTestFixture, replace) {
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
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeReplaced()) {
    EXPECT_TRUE(isReified());
    EXPECT_FALSE(varNode(reifiedIdentifier).isFixed());
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (!varNode(identifier).isFixed()) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }
  EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedIdentifier).isFixed());
      const bool actual = varNode(reifiedIdentifier).inDomain({false});
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
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

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
