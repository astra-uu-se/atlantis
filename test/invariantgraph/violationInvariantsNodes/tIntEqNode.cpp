#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntEqNodeTestFixture : public NodeTestBase<IntEqNode> {
 public:
  VarNodeId aVarNodeId{NULL_NODE_ID};
  VarNodeId bVarNodeId{NULL_NODE_ID};
  VarNodeId reifiedVarNodeId{NULL_NODE_ID};
  std::string reifiedIdentifier{"reified"};

  bool isViolating() {
    return varNode(aVarNodeId).lowerBound() != varNode(bVarNodeId).lowerBound();
  }

  bool isViolating(propagation::Solver& solver) {
    const Int aVal = varNode(aVarNodeId).isFixed()
                         ? varNode(aVarNodeId).lowerBound()
                         : solver.currentValue(varId(aVarNodeId));
    const Int bVal = varNode(bVarNodeId).isFixed()
                         ? varNode(bVarNodeId).lowerBound()
                         : solver.currentValue(varId(bVarNodeId));

    return aVal != bVal;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    aVarNodeId = retrieveIntVarNode(5, 10, "a");
    bVarNodeId = retrieveIntVarNode(2, 7, "b");

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

TEST_P(IntEqNodeTestFixture, construction) {
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

TEST_P(IntEqNodeTestFixture, application) {
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
  EXPECT_EQ(solver.searchVars().size(), 2);

  // aVarNodeId, bVarNodeId and the violation
  EXPECT_EQ(solver.numVars(), 3);

  // equal
  EXPECT_EQ(solver.numInvariants(), 1);

  EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
  EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
}

TEST_P(IntEqNodeTestFixture, propagation) {
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
  solver.close();

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
    IntEqNodeTest, IntEqNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
