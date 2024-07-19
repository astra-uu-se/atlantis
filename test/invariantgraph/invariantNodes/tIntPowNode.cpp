#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntPowNodeTestFixture : public NodeTestBase<IntPowNode> {
 public:
  VarNodeId baseVarNodeId{NULL_NODE_ID};
  VarNodeId exponentVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int int_exp(Int baseVal, Int exponentVal) {
    if (exponentVal == 0) {
      return 1;
    }
    if (exponentVal == 1) {
      return baseVal;
    }
    if (exponentVal < 0) {
      EXPECT_NE(baseVal, 0);
      if (baseVal == 1) {
        return 1;
      }
      if (baseVal == -1) {
        return exponentVal % 2 == 0 ? 1 : -1;
      }
      return 0;
    }
    Int result = 1;
    for (int i = 0; i < exponentVal; i++) {
      result *= baseVal;
    }
    return result;
  }

  Int computeOutput() {
    const Int baseVal = varNode(baseVarNodeId).lowerBound();
    const Int exponentVal = varNode(exponentVarNodeId).lowerBound();
    return int_exp(baseVal, exponentVal);
  }

  Int computeOutput(propagation::Solver& solver) {
    const Int baseVal = varNode(baseVarNodeId).isFixed()
                            ? varNode(baseVarNodeId).lowerBound()
                            : solver.currentValue(varId(baseVarNodeId));
    const Int exponentVal = varNode(exponentVarNodeId).isFixed()
                                ? varNode(exponentVarNodeId).lowerBound()
                                : solver.currentValue(varId(exponentVarNodeId));
    return int_exp(baseVal, exponentVal);
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    baseVarNodeId = retrieveIntVarNode(0, 10, "base");
    exponentVarNodeId = retrieveIntVarNode(0, 10, "exponent");
    outputVarNodeId = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(baseVarNodeId, exponentVarNodeId, outputVarNodeId);
  }
};

TEST_P(IntPowNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().base(), baseVarNodeId);
  EXPECT_EQ(invNode().exponent(), exponentVarNodeId);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntPowNodeTestFixture, application) {
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

  // baseVarNodeId and exponentVarNodeId
  EXPECT_EQ(solver.searchVars().size(), 2);

  // baseVarNodeId, exponentVarNodeId and outputVarNodeId
  EXPECT_EQ(solver.numVars(), 3);

  // intPow
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntPowNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);
  _invariantGraph->close(solver);

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(solver);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId :
       std::array<VarNodeId, 2>{baseVarNodeId, exponentVarNodeId}) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

    if (inputVals.at(0) != 0 || inputVals.at(1) > 0) {
      const Int actual = solver.currentValue(outputId);
      const Int expected = computeOutput(solver);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(IntPowNodeTest, IntPowNodeTestFixture,
                        ::testing::Values(ParamData{
                            InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
