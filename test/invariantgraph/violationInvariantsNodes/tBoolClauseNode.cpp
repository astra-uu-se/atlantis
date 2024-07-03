#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolClauseNodeTestFixture : public NodeTestBase<BoolClauseNode> {
 public:
  std::vector<VarNodeId> asInputs;
  std::vector<VarNodeId> bsInputs;
  VarNodeId reified{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  Int numAs{2};
  Int numBs{2};

  bool isViolating(propagation::Solver& solver) {
    std::vector<bool> asVals;
    asVals.reserve(asInputs.size());
    for (const auto& a : asInputs) {
      if (varNode(a).isFixed()) {
        asVals.emplace_back(varNode(a).inDomain(bool{true}));
      } else {
        asVals.emplace_back(solver.currentValue(varId(a)) == 0);
      }
    }
    std::vector<bool> bsVals;
    bsVals.reserve(bsInputs.size());
    for (const auto& b : bsInputs) {
      if (varNode(b).isFixed()) {
        bsVals.emplace_back(varNode(b).inDomain(bool{true}));
      } else {
        bsVals.emplace_back(solver.currentValue(varId(b)) == 0);
      }
    }
    const bool aClause =
        std::any_of(asVals.begin(), asVals.end(), [](bool v) { return v; });
    const bool bClause =
        std::any_of(bsVals.begin(), bsVals.end(), [](bool v) { return !v; });
    return !aClause && !bClause;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    asInputs.clear();
    bsInputs.clear();
    numAs = 2;
    numBs = 2;
    if (shouldBeReplaced()) {
      if (_paramData.data == 0) {
        numAs = 0;
      } else if (_paramData.data == 1) {
        numBs = 0;
      }
    }
    asInputs.reserve(numAs);
    bsInputs.reserve(numBs);

    for (Int i = 0; i < numAs; ++i) {
      asInputs.emplace_back(retrieveBoolVarNode("a_" + std::to_string(i)));
      if (shouldBeSubsumed() && (_paramData.data != 0 || i != 0)) {
        varNode(asInputs.at(i)).fixToValue(!shouldFail());
      }
    }
    for (Int i = 0; i < numBs; ++i) {
      bsInputs.emplace_back(retrieveBoolVarNode("b_" + std::to_string(i)));
      if (shouldBeSubsumed() && (_paramData.data != 1 || i != 0)) {
        varNode(bsInputs.at(i)).fixToValue(shouldFail());
      }
    }

    if (isReified()) {
      reified = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(std::vector<VarNodeId>(asInputs),
                          std::vector<VarNodeId>(bsInputs), reified);
    } else {
      if (shouldHold()) {
        createInvariantNode(std::vector<VarNodeId>(asInputs),
                            std::vector<VarNodeId>(bsInputs), true);
      } else {
        createInvariantNode(std::vector<VarNodeId>(asInputs),
                            std::vector<VarNodeId>(bsInputs), false);
      }
    }
  }
};

TEST_P(BoolClauseNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numAs + numBs);
  for (Int i = 0; i < numAs; ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), asInputs.at(i));
  }
  for (Int i = 0; i < numBs; ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(numAs + i), bsInputs.at(i));
  }

  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
  }
}

TEST_P(BoolClauseNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedIdentifier).isFixed());
      bool expected = false;
      for (const auto& a : asInputs) {
        expected = expected && varNode(a).inDomain(bool{false});
      }
      for (const auto& b : bsInputs) {
        expected = expected && varNode(b).inDomain(bool{true});
      }
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

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : asInputs) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
    }
  }
  for (const auto& inputVarNodeId : bsInputs) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_EQ(inputVars.size() <= 1, shouldBeSubsumed());
  if (shouldBeSubsumed()) {
    EXPECT_EQ(isViolating(solver), shouldFail());
    return;
  }

  const propagation::VarId violVarId =
      isReified() ? varId(reifiedIdentifier)
                  : _invariantGraph->totalViolationVarId();

  EXPECT_NE(violVarId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(violVarId);
    solver.endProbe();

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
