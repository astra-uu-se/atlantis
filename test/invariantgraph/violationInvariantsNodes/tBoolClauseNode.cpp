#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolClauseNodeTestFixture : public NodeTestBase<BoolClauseNode> {
 public:
  std::vector<VarNodeId> asInputVarNodeIds;
  std::vector<VarNodeId> bsInputVarNodeIds;
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  Int numAs{2};
  Int numBs{2};

  bool isViolating() {
    for (const auto& a : asInputVarNodeIds) {
      for (const auto& b : bsInputVarNodeIds) {
        if (a == b) {
          return false;
        }
      }
    }
    for (const auto& a : asInputVarNodeIds) {
      if (varNode(a).inDomain(bool{true})) {
        return false;
      }
    }
    for (const auto& b : bsInputVarNodeIds) {
      if (varNode(b).inDomain(bool{false})) {
        return false;
      }
    }
    return false;
  }

  bool isViolating(propagation::Solver& solver) {
    for (const auto& a : asInputVarNodeIds) {
      for (const auto& b : bsInputVarNodeIds) {
        if (a == b) {
          return false;
        }
      }
    }
    for (const auto& a : asInputVarNodeIds) {
      if (varNode(a).isFixed()) {
        if (varNode(a).inDomain(bool{true})) {
          return false;
        }
      } else if (solver.currentValue(varId(a)) == 0) {
        return false;
      }
    }
    for (const auto& b : bsInputVarNodeIds) {
      if (varNode(b).isFixed()) {
        if (varNode(b).inDomain(bool{false})) {
          return false;
        }
      } else if (solver.currentValue(varId(b)) > 0) {
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
    bsInputVarNodeIds.reserve(numBs);

    for (Int i = 0; i < numAs; ++i) {
      asInputVarNodeIds.emplace_back(
          retrieveBoolVarNode("a_" + std::to_string(i)));
      if (shouldBeSubsumed() && (_paramData.data != 0 || i != 0)) {
        varNode(asInputVarNodeIds.at(i)).fixToValue(!shouldFail());
      }
    }
    for (Int i = 0; i < numBs; ++i) {
      bsInputVarNodeIds.emplace_back(
          retrieveBoolVarNode("b_" + std::to_string(i)));
      if (shouldBeSubsumed() && (_paramData.data != 1 || i != 0)) {
        varNode(bsInputVarNodeIds.at(i)).fixToValue(shouldFail());
      }
    }

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>(asInputVarNodeIds),
                          std::vector<VarNodeId>(bsInputVarNodeIds),
                          reifiedVarNodeId);
    } else {
      if (shouldHold()) {
        createInvariantNode(std::vector<VarNodeId>(asInputVarNodeIds),
                            std::vector<VarNodeId>(bsInputVarNodeIds), true);
      } else {
        createInvariantNode(std::vector<VarNodeId>(asInputVarNodeIds),
                            std::vector<VarNodeId>(bsInputVarNodeIds), false);
      }
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
  invNode().updateState(*_invariantGraph);
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
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(BoolClauseNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  if (shouldBeSubsumed()) {
    const bool expected = isViolating(solver);
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
  for (const auto& inputVarNodeId : asInputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }
  for (const auto& inputVarNodeId : bsInputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_EQ(inputVarIds.size() <= 1, shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    EXPECT_EQ(isViolating(solver), shouldFail());
    return;
  }

  const propagation::VarId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(violVarId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

    const bool actual = solver.currentValue(violVarId) > 0;
    const bool expected = isViolating(solver);

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
