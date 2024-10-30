#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolClauseNodeTestFixture : public NodeTestBase<BoolClauseNode> {
 public:
  std::vector<VarNodeId> asInputVarNodeIds;
  std::vector<std::string> asIdentifiers;
  std::vector<VarNodeId> bsInputVarNodeIds;
  std::vector<std::string> bsIdentifiers;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  Int numAs{2};
  Int numBs{2};

  bool isViolating(bool isRegistered = false) {
    if (isRegistered) {
      for (const auto& a : asIdentifiers) {
        for (const auto& b : bsIdentifiers) {
          if (a == b) {
            return false;
          }
        }
      }
      for (const auto& a : asIdentifiers) {
        if (varNode(a).isFixed()) {
          if (varNode(a).inDomain(bool{true})) {
            return false;
          }
        } else if (_solver->currentValue(varId(a)) == 0) {
          return false;
        }
      }
      for (const auto& b : bsIdentifiers) {
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
    for (const auto& a : asIdentifiers) {
      for (const auto& b : bsIdentifiers) {
        if (varNode(a).varNodeId() == varNode(b).varNodeId()) {
          return false;
        }
      }
    }
    for (const auto& a : asIdentifiers) {
      const auto& vNode = varNode(a);
      if (vNode.inDomain(bool{true})) {
        return false;
      }
    }
    for (const auto& b : bsIdentifiers) {
      const auto& vNode = varNode(b);
      if (vNode.inDomain(bool{false})) {
        return false;
      }
    }
    return true;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    asInputVarNodeIds.clear();
    bsInputVarNodeIds.clear();
    numAs = 2;
    numBs = 2;
    if (shouldBeReplaced()) {
      if (_paramData.data == 0) {
        numAs = 0;
      } else if (_paramData.data == 1) {
        numBs = 0;
      }
    }
    asInputVarNodeIds.reserve(numAs);
    asIdentifiers.reserve(numAs);
    bsInputVarNodeIds.reserve(numBs);
    bsIdentifiers.reserve(numBs);

    for (Int i = 0; i < numAs; ++i) {
      asIdentifiers.emplace_back("a_" + std::to_string(i));
      asInputVarNodeIds.emplace_back(retrieveBoolVarNode(asIdentifiers.back()));
      if (shouldBeSubsumed() && (_paramData.data != 0 || i != 0)) {
        varNode(asInputVarNodeIds.back()).fixToValue(!shouldFail());
      }
    }
    for (Int i = 0; i < numBs; ++i) {
      bsIdentifiers.emplace_back("b_" + std::to_string(i));
      bsInputVarNodeIds.emplace_back(retrieveBoolVarNode(bsIdentifiers.back()));
      if (shouldBeSubsumed() && (_paramData.data != 1 || i != 0)) {
        varNode(bsInputVarNodeIds.back()).fixToValue(shouldFail());
      }
    }

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(
          *_invariantGraph, std::vector<VarNodeId>(asInputVarNodeIds),
          std::vector<VarNodeId>(bsInputVarNodeIds), reifiedVarNodeId);
    } else {
      createInvariantNode(
          *_invariantGraph, std::vector<VarNodeId>(asInputVarNodeIds),
          std::vector<VarNodeId>(bsInputVarNodeIds), shouldHold());
    }
  }
};

TEST_P(BoolClauseNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numAs + numBs);
  for (Int i = 0; i < numAs; ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), asInputVarNodeIds.at(i));
  }
  for (Int i = 0; i < numBs; ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(numAs + i),
              bsInputVarNodeIds.at(i));
  }

  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  }
}

TEST_P(BoolClauseNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedIdentifier).isFixed());
      const bool expected = isViolating();
      const bool actual = varNode(reifiedIdentifier).inDomain(bool{false});
      EXPECT_EQ(expected, actual);
    }
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_FALSE(varNode(reifiedIdentifier).isFixed());
    }
  }
}

TEST_P(BoolClauseNodeTestFixture, replace) {
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

TEST_P(BoolClauseNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

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

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& identifier : asIdentifiers) {
    if (!varNode(identifier).isFixed()) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }
  for (const auto& identifier : bsIdentifiers) {
    if (!varNode(identifier).isFixed()) {
      EXPECT_NE(varId(identifier), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifier));
    }
  }

  EXPECT_EQ(inputVarIds.size() <= 1, shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    EXPECT_EQ(isViolating(true), shouldFail());
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
                                ViolationInvariantType::CONSTANT_FALSE, int{0}},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_FALSE, int{1}},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
