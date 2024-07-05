#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class BoolLtNodeTestFixture : public NodeTestBase<BoolLtNode> {
 public:
  VarNodeId aVarNodeId{NULL_NODE_ID};
  VarNodeId bVarNodeId{NULL_NODE_ID};
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  bool isViolating() {
    const VarNode& aNode = varNode(aVarNodeId);
    const VarNode& bNode = varNode(bVarNodeId);

    const bool aVal = aNode.inDomain(bool{true});
    const bool bVal = bNode.inDomain(bool{true});

    // !(a < b) <=> a >= b
    return aVal || !bVal;
  }

  bool isViolating(propagation::Solver& solver) {
    const bool aVal = varNode(aVarNodeId).isFixed()
                          ? varNode(aVarNodeId).inDomain(bool{true})
                          : solver.currentValue(varId(aVarNodeId)) == 0;
    const bool bVal = varNode(bVarNodeId).isFixed()
                          ? varNode(bVarNodeId).inDomain(bool{true})
                          : solver.currentValue(varId(bVarNodeId)) == 0;
    // !(a < b) <=> a >= b
    return aVal || !bVal;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    aVarNodeId = retrieveBoolVarNode("a");
    if (shouldBeSubsumed() && _paramData.data == 2) {
      bVarNodeId = aVarNodeId;
    } else {
      bVarNodeId = retrieveBoolVarNode("b");
    }

    if (shouldBeReplaced()) {
      if (isReified()) {
        if (_paramData.data == 0) {
          varNode(aVarNodeId).fixToValue(bool{false});
        } else {
          varNode(bVarNodeId).fixToValue(bool{true});
        }
      }
    }
    if (shouldBeSubsumed()) {
      if (isReified() || shouldFail()) {
        if (_paramData.data == 0) {
          varNode(aVarNodeId).fixToValue(bool{true});
        } else {
          varNode(bVarNodeId).fixToValue(bool{false});
        }
      }
    }

    if (isReified()) {
      reifiedVarNodeId = retrieveBoolVarNode(reifiedIdentifier);
      createInvariantNode(aVarNodeId, bVarNodeId, reifiedVarNodeId);
    } else if (shouldHold()) {
      createInvariantNode(aVarNodeId, bVarNodeId, true);
    } else {
      createInvariantNode(aVarNodeId, bVarNodeId, false);
    }
  }
};

TEST_P(BoolLtNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().a(), aVarNodeId);
  EXPECT_EQ(invNode().b(), bVarNodeId);
  expectInputTo(invNode());
  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reifiedVarNodeId);
  }
}

TEST_P(BoolLtNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // aVarNodeId and bVarNodeId
  EXPECT_LE(solver.searchVars().size(), 2);

  // aVarNodeId, bVarNodeId and the violation
  EXPECT_LE(solver.numVars(), 3);

  // equal
  EXPECT_EQ(solver.numInvariants(), 1);

  EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
  EXPECT_EQ(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 1);
}

TEST_P(BoolLtNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
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
  }
}

TEST_P(BoolLtNodeTestFixture, replace) {
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

TEST_P(BoolLtNodeTestFixture, propagation) {
  if (shouldBeMadeImplicit()) {
    return;
  }
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  if (shouldBeSubsumed()) {
    const bool expected = isViolating();
    if (isReified()) {
      EXPECT_TRUE(varNode(reifiedIdentifier).isFixed());
      const bool actual = varNode(reifiedIdentifier).inDomain(bool{false});
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
  for (const auto& inputVarNodeId :
       std::array<VarNodeId, 2>{aVarNodeId, bVarNodeId}) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
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
    BoolLtNodeTest, BoolLtNodeTestFixture,
    ::testing::Values(ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED, 0},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::REIFIED, 1},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_FALSE, 2},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 0},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 1},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::REIFIED, 2}));

}  // namespace atlantis::testing
