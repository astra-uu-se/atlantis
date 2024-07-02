#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& inputVals, size_t asSize) {
  for (size_t i = 0; i < asSize; ++i) {
    if (inputVals.at(i) != 0) {
      return true;
    }
  }
  for (size_t i = asSize; i < inputVals.size(); ++i) {
    if (inputVals.at(i) == 0) {
      return true;
    }
  }
  return false;
}

class BoolClauseNodeTestFixture : public NodeTestBase<BoolClauseNode> {
 public:
  VarNodeId a1{NULL_NODE_ID};
  VarNodeId a2{NULL_NODE_ID};
  VarNodeId b1{NULL_NODE_ID};
  VarNodeId b2{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a1 = retrieveBoolVarNode("a1");
    a2 = retrieveBoolVarNode("a2");
    b1 = retrieveBoolVarNode("b1");
    b2 = retrieveBoolVarNode("b2");

    std::vector<VarNodeId> as{a1, a2};
    std::vector<VarNodeId> bs{b1, b2};

    if (isReified()) {
      reified = retrieveBoolVarNode("reified");
      createInvariantNode(std::move(as), std::move(bs), reified);
    } else {
      if (shouldHold()) {
        createInvariantNode(std::move(as), std::move(bs), true);
      } else {
        createInvariantNode(std::move(as), std::move(bs), false);
      }
    }
  }
};

TEST_P(BoolClauseNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().as().size(), 2);
  EXPECT_EQ(invNode().as().at(0), a1);
  EXPECT_EQ(invNode().as().at(1), a2);

  EXPECT_EQ(invNode().bs().size(), 2);
  EXPECT_EQ(invNode().bs().at(0), b1);
  EXPECT_EQ(invNode().bs().at(1), b2);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(),
            invNode().as().size() + invNode().bs().size());

  std::vector<VarNodeId> expectedVars(invNode().as());
  for (const auto& varNodeId : invNode().bs()) {
    expectedVars.emplace_back(varNodeId);
  }
  EXPECT_EQ(expectedVars, invNode().staticInputVarNodeIds());

  if (!isReified()) {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
  }
}

TEST_P(BoolClauseNodeTestFixture, application) {
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

  // a1, a2, b1 and b2
  EXPECT_EQ(solver.searchVars().size(), 4);

  // a1, a2, b1, b2, sum
  EXPECT_EQ(solver.numVars(), 5);

  // linear
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(BoolClauseNodeTestFixture, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  const size_t asSize = invNode().as().size();
  EXPECT_EQ(asSize, 2);
  std::vector<propagation::VarId> inputVars;
  for (const VarNodeId& inputVarNodeId : invNode().as()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
    solver.updateBounds(varId(inputVarNodeId), 0, 5, true);
  }
  EXPECT_EQ(invNode().bs().size(), 2);
  for (const VarNodeId& inputVarNodeId : invNode().bs()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
    solver.updateBounds(varId(inputVarNodeId), 0, 5, true);
  }

  EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
  const propagation::VarId violationId =
      invNode().violationVarId(*_invariantGraph);

  solver.close();

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(violationId);
    solver.endProbe();

    const Int actual = solver.currentValue(violationId) > 0;
    const Int expected = isViolating(inputVals, asSize);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    BoolClauseNodeTest, BoolClauseNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
