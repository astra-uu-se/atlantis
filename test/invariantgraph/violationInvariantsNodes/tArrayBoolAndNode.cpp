#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

class ArrayBoolAndNodeTestFixture : public NodeTestBase<ArrayBoolAndNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};
  Int numInputs = 4;

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      for (const auto& identifier : inputIdentifiers) {
        if (varNode(identifier).isFixed()) {
          if (varNode(identifier).inDomain(bool{false})) {
            return true;
          }
        } else {
          EXPECT_NE(varId(identifier), propagation::NULL_ID);
          if (_solver->currentValue(varId(identifier)) > 0) {
            return true;
          }
        }
      }
      return false;
    }
    for (const auto& identifier : inputIdentifiers) {
      if (varNode(identifier).inDomain(bool{false})) {
        return true;
      }
    }
    return false;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.clear();
    inputVarNodeIds.reserve(numInputs);
    inputIdentifiers.clear();
    inputIdentifiers.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputIdentifiers.emplace_back("input_" + std::to_string(i));
      inputVarNodeIds.emplace_back(
          retrieveBoolVarNode(inputIdentifiers.back()));
    }

    if (shouldBeSubsumed()) {
      for (const auto& identifier : inputIdentifiers) {
        varNode(identifier).fixToValue(!shouldFail());
      }
    } else if (shouldBeReplaced()) {
      for (size_t i = 1; i < inputIdentifiers.size(); ++i) {
        varNode(inputIdentifiers.at(i)).fixToValue(!shouldFail());
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

TEST_P(ArrayBoolAndNodeTestFixture, construction) {
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

TEST_P(ArrayBoolAndNodeTestFixture, application) {
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

TEST_P(ArrayBoolAndNodeTestFixture, updateState) {
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

TEST_P(ArrayBoolAndNodeTestFixture, replace) {
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

TEST_P(ArrayBoolAndNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeReplaced() && isReified()) {
    EXPECT_EQ(varId(reifiedIdentifier), propagation::NULL_ID);
    EXPECT_FALSE(varNode(reifiedIdentifier).isFixed());
    return;
  }

  const propagation::VarId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

  if (shouldBeSubsumed()) {
    EXPECT_TRUE(shouldBeSubsumed() || shouldBeReplaced());
    const bool expected = isViolating(true);
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

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& identifier : inputIdentifiers) {
    if (!varNode(identifier).isFixed()) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }

  EXPECT_EQ(inputVarIds.empty(), shouldBeSubsumed());

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(inputVarIds);

  while (increaseNextVal(inputVarIds, inputVals)) {
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
    ArrayBoolAndNodeTest, ArrayBoolAndNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
