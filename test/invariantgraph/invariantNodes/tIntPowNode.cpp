#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntPowNodeTestFixture : public NodeTestBase<IntPowNode> {
 public:
  VarNodeId base{NULL_NODE_ID};
  VarNodeId exponent{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(propagation::Solver& solver) {
    const Int baseVal = varNode(base).isFixed()
                            ? varNode(base).lowerBound()
                            : solver.currentValue(varId(base));
    const Int exponentVal = varNode(exponent).isFixed()
                                ? varNode(exponent).lowerBound()
                                : solver.currentValue(varId(exponent));
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

  void SetUp() override {
    NodeTestBase::SetUp();
    base = retrieveIntVarNode(0, 10, "base");
    exponent = retrieveIntVarNode(0, 10, "exponent");
    output = retrieveIntVarNode(0, 10, "output");

    createInvariantNode(base, exponent, output);
  }
};

TEST_P(IntPowNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().base(), base);
  EXPECT_EQ(invNode().exponent(), exponent);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
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

  // base and exponent
  EXPECT_EQ(solver.searchVars().size(), 2);

  // base, exponent and output
  EXPECT_EQ(solver.numVars(), 3);

  // intPow
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntPowNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

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
