#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

using ::testing::ContainerEq;

static bool isViolating(const std::vector<Int>& values) {
  return std::all_of(values.begin(), values.end(),
                     [&](const Int val) { return val > 0; });
}

class ArrayBoolOrNodeTestFixture : public NodeTestBase<ArrayBoolOrNode> {
 public:
  VarNodeId x1{NULL_NODE_ID};
  VarNodeId x2{NULL_NODE_ID};
  VarNodeId x3{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = retrieveBoolVarNode("x1");
    x2 = retrieveBoolVarNode("x2");
    x3 = retrieveBoolVarNode("x3");

    std::vector<VarNodeId> inputs{x1, x2, x3};

    if (isReified()) {
      reified = retrieveBoolVarNode("reified");
      createInvariantNode(std::move(inputs), reified);
    } else if (shouldHold()) {
      createInvariantNode(std::move(inputs), true);
    } else {
      createInvariantNode(std::move(inputs), false);
    }
  }
};

TEST_P(ArrayBoolOrNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
  std::vector<VarNodeId> expectedVars{x1, x2, x3};
  EXPECT_EQ(invNode().staticInputVarNodeIds(), expectedVars);
  EXPECT_THAT(expectedVars, ContainerEq(invNode().staticInputVarNodeIds()));
  if (isReified()) {
    EXPECT_TRUE(invNode().isReified());
    EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
  } else {
    EXPECT_FALSE(invNode().isReified());
    EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
  }
}

TEST_P(ArrayBoolOrNodeTestFixture, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // x1, x2, and x3
  EXPECT_EQ(solver.searchVars().size(), 3);

  // x1, x2, x3 and reified
  EXPECT_EQ(solver.numVars(), 4);

  // minSparse
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayBoolOrNodeTestFixture, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputVars;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
    solver.updateBounds(varId(inputVarNodeId), 0, 10, true);
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
    const Int expected = isViolating(inputVals);

    if (!shouldFail()) {
      EXPECT_EQ(actual, expected);
    } else {
      EXPECT_NE(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayBoolOrNodeTest, ArrayBoolOrNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::REPLACE,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{InvariantNodeAction::SUBSUME,
                                ViolationInvariantType::CONSTANT_TRUE},
                      ParamData{ViolationInvariantType::CONSTANT_FALSE},
                      ParamData{ViolationInvariantType::REIFIED}));

}  // namespace atlantis::testing
