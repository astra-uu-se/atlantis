#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModNodeTestFixture : public NodeTestBase<IntModNode> {
 public:
  VarNodeId numeratorVarNodeId{NULL_NODE_ID};
  VarNodeId denominatorVarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput() {
    const Int numerator = varNode(numeratorVarNodeId).lowerBound();
    const Int denominator = varNode(denominatorVarNodeId).lowerBound();
    return denominator != 0 ? numerator % denominator : 0;
  }

  Int computeOutput(propagation::Solver& solver) {
    const Int numerator = varNode(numeratorVarNodeId).isFixed()
                              ? varNode(numeratorVarNodeId).lowerBound()
                              : solver.currentValue(varId(numeratorVarNodeId));
    const Int denominator =
        varNode(denominatorVarNodeId).isFixed()
            ? varNode(denominatorVarNodeId).lowerBound()
            : solver.currentValue(varId(denominatorVarNodeId));
    return denominator != 0 ? numerator % denominator : 0;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    numeratorVarNodeId = retrieveIntVarNode(0, 6, "numerator");
    denominatorVarNodeId = retrieveIntVarNode(1, 10, "denominator");
    outputVarNodeId = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(numeratorVarNodeId, denominatorVarNodeId,
                        outputVarNodeId);
  }
};

TEST_P(IntModNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().numerator(), numeratorVarNodeId);
  EXPECT_EQ(invNode().denominator(), denominatorVarNodeId);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntModNodeTestFixture, application) {
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

  // numeratorVarNodeId and denominatorVarNodeId
  EXPECT_EQ(solver.searchVars().size(), 2);

  // numeratorVarNodeId, denominatorVarNodeId and outputVarNodeId
  EXPECT_EQ(solver.numVars(), 3);

  // intMod
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntModNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(solver);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId :
       std::array<VarNodeId, 2>{numeratorVarNodeId, denominatorVarNodeId}) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputVarIds.size(), 2);

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

    if (inputVals.at(1) != 0) {
      const Int actual = solver.currentValue(outputId);
      const Int expected = computeOutput(solver);
      EXPECT_EQ(actual, expected);
    }
  }
}

INSTANTIATE_TEST_CASE_P(IntModNodeTest, IntModNodeTestFixture,
                        ::testing::Values(ParamData{
                            InvariantNodeAction::NONE}));

}  // namespace atlantis::testing
