#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLeNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntLeNodeTestFixture : public NodeTestBase<IntLeNode> {
 public:
  VarNodeId aVarNodeId{NULL_NODE_ID};
  VarNodeId bVarNodeId{NULL_NODE_ID};
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      const Int aVal = varNode(aVarNodeId).isFixed()
                           ? varNode(aVarNodeId).lowerBound()
                           : _solver->currentValue(varId(aVarNodeId));
      const Int bVal = varNode(bVarNodeId).isFixed()
                           ? varNode(bVarNodeId).lowerBound()
                           : _solver->currentValue(varId(bVarNodeId));

      return aVal > bVal;
    }
    return varNode(aVarNodeId).lowerBound() > varNode(bVarNodeId).lowerBound();
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    aVarNodeId = retrieveIntVarNode(-5, 5, "a");
    bVarNodeId = retrieveIntVarNode(-5, 5, "b");
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
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(*_invariantGraph, aVarNodeId, bVarNodeId,
                          reifiedVarNodeId);
    } else {
      createInvariantNode(*_invariantGraph, aVarNodeId, bVarNodeId,
                          shouldHold());
    }
  }
};

TEST_P(IntLeNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().a(), aVarNodeId);
  EXPECT_EQ(invNode().b(), bVarNodeId);

  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  }
}

TEST_P(IntLeNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_EQ(invNode().violationVarId(), propagation::NULL_ID);
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_NE(invNode().violationVarId(), propagation::NULL_ID);
  invNode().registerNode();
  _solver->close();

  // aVarNodeId and bVarNodeId
  EXPECT_EQ(_solver->searchVars().size(), 2);

  // aVarNodeId, bVarNodeId and the violation
  EXPECT_EQ(_solver->numVars(), 3);

  // less equal
  EXPECT_EQ(_solver->numInvariants(), 1);

  EXPECT_GE(_solver->upperBound(invNode().violationVarId()), 0);
}

TEST_P(IntLeNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    // TODO: disabled for the MZN challange. This should be computed by Gecode.
    /*
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedVarNodeId).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedVarNodeId).inDomain(bool{false});
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
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    // TODO: disabled for the MZN challange. This should be computed by Gecode
    /*
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedIdentifier).isFixed());
      const bool actual = varNode(reifiedIdentifier).inDomain({false});
      EXPECT_EQ(expected, actual);
    }
    */
    if (shouldHold()) {
      EXPECT_FALSE(expected);
    }
    // TODO: disabled for the MZN challange. This should be computed by Gecode
    /*
    if (shouldFail()) {
      EXPECT_TRUE(expected);
    }
    */
    return;
  }

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& inputVarNodeId :
       std::array<VarNodeId, 2>{aVarNodeId, bVarNodeId}) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
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
