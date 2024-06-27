#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_int_maximum.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMaximumNodeTestFixture
    : public NodeTestBase<ArrayIntMaximumNode> {
 public:
  std::vector<VarNodeId> inputVars;
  VarNodeId outputVar{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  bool shouldBeSubsumed() const { return _mode == 1; }
  bool shouldBeReplaced() const { return _mode == 2; }

  Int computeOutput(propagation::Solver& solver) {
    Int val = std::numeric_limits<Int>::min();
    for (const auto& input : inputVars) {
      if (varId(input) == propagation::NULL_ID) {
        val = std::max(val, varNode(input).upperBound());
      } else {
        val = std::max(val, solver.currentValue(varId(input)));
      }
    }
    return val;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (shouldBeSubsumed()) {
      inputVars = {retrieveIntVarNode(-5, 2, "x1"),
                   retrieveIntVarNode(-2, 5, "x2"),
                   retrieveIntVarNode(5, 5, "x3")};

    } else if (shouldBeReplaced()) {
      inputVars = {retrieveIntVarNode(-2, 5, "x1"),
                   retrieveIntVarNode(-5, -2, "x2"),
                   retrieveIntVarNode(-2, -2, "x3")};
    } else {
      inputVars = {retrieveIntVarNode(0, 5, "x1"),
                   retrieveIntVarNode(2, 2, "x2"),
                   retrieveIntVarNode(-5, 0, "x3")};
    }
    outputVar = retrieveIntVarNode(-5, 5, outputIdentifier);

    createInvariantNode(std::vector<VarNodeId>{inputVars}, outputVar);
  }
};

TEST_P(ArrayIntMaximumNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVars.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVars.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVar);
}

TEST_P(ArrayIntMaximumNodeTestFixture, application) {
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

  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    lb = std::max(lb, solver.lowerBound(varId(inputVarNodeId)));
    ub = std::max(ub, solver.upperBound(varId(inputVarNodeId)));
  }

  EXPECT_EQ(solver.lowerBound(varId(outputVar)), lb);
  EXPECT_EQ(solver.upperBound(varId(outputVar)), ub);

  // x1, x2, and x3
  EXPECT_EQ(solver.searchVars().size(), 3);

  // x1, x2 and outputVar
  EXPECT_EQ(solver.numVars(), 4);

  // maxSparse
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayIntMaximumNodeTestFixture, updateState) {
  Int minVal = std::numeric_limits<Int>::max();
  Int maxVal = std::numeric_limits<Int>::min();
  for (const auto& var : inputVars) {
    minVal = std::min(minVal, _invariantGraph->varNode(var).lowerBound());
    maxVal = std::max(maxVal, _invariantGraph->varNode(var).upperBound());
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(_invariantGraph->varNode(outputVar).isFixed());
  } else {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_LE(minVal, _invariantGraph->varNode(outputVar).lowerBound());
  EXPECT_GE(maxVal, _invariantGraph->varNode(outputVar).upperBound());
}

TEST_P(ArrayIntMaximumNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else if (invNode().state() == InvariantNodeState::ACTIVE) {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_FALSE(invNode().replace(*_invariantGraph));
  }
}

TEST_P(ArrayIntMaximumNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  VarNode& outputNode = varNode(outputIdentifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(solver);
    EXPECT_EQ(expected, actual);
    return;
  }
  EXPECT_NE(varId(outputIdentifier), propagation::NULL_ID);

  const propagation::VarId outputId = varId(outputIdentifier);

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int expected = computeOutput(solver);
    const Int actual = solver.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_CASE_P(ArrayIntMaximumNodeTest, ArrayIntMaximumNodeTestFixture,
                        ::testing::Values(0, 1, 2, 3));

}  // namespace atlantis::testing
