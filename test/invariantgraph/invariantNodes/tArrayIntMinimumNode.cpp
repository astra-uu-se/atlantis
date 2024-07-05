#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_int_minimum.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMinimumNodeTestFixture
    : public NodeTestBase<ArrayIntMinimumNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput() {
    Int val = std::numeric_limits<Int>::max();
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      val = std::min(val, varNode(inputVarNodeId).upperBound());
    }
    return val;
  }

  Int computeOutput(propagation::Solver& solver) {
    Int val = std::numeric_limits<Int>::max();
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      if (varNode(inputVarNodeId).isFixed() ||
          varId(inputVarNodeId) == propagation::NULL_ID) {
        val = std::min(val, varNode(inputVarNodeId).upperBound());
      } else {
        val = std::min(val, solver.currentValue(varId(inputVarNodeId)));
      }
    }
    return val;
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (shouldBeSubsumed()) {
      inputVarNodeIds = {retrieveIntVarNode(-2, 5, "x1"),
                         retrieveIntVarNode(-5, 2, "x2"),
                         retrieveIntVarNode(-5, -5, "x3")};

    } else if (shouldBeReplaced()) {
      inputVarNodeIds = {retrieveIntVarNode(-5, 2, "x1"),
                         retrieveIntVarNode(2, 5, "x2"),
                         retrieveIntVarNode(2, 2, "x3")};
    } else {
      inputVarNodeIds = {retrieveIntVarNode(-5, 0, "x1"),
                         retrieveIntVarNode(-2, -2, "x2"),
                         retrieveIntVarNode(0, 5, "x3")};
    }
    outputVarNodeId = retrieveIntVarNode(-5, 5, outputIdentifier);

    createInvariantNode(std::vector<VarNodeId>{inputVarNodeIds},
                        outputVarNodeId);
  }
};

TEST_P(ArrayIntMinimumNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(ArrayIntMinimumNodeTestFixture, application) {
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

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::max();

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    lb = std::min(lb, solver.lowerBound(varId(inputVarNodeId)));
    ub = std::min(ub, solver.upperBound(varId(inputVarNodeId)));
  }

  EXPECT_EQ(solver.lowerBound(varId(outputVarNodeId)), lb);
  EXPECT_EQ(solver.upperBound(varId(outputVarNodeId)), ub);

  // x1, x2, and x3
  EXPECT_EQ(solver.searchVars().size(), 3);

  // x1, x2 and outputVarNodeId
  EXPECT_EQ(solver.numVars(), 4);

  // maxSparse
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayIntMinimumNodeTestFixture, updateState) {
  Int minVal = std::numeric_limits<Int>::min();
  Int maxVal = std::numeric_limits<Int>::max();
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    minVal =
        std::min(minVal, _invariantGraph->varNode(inputVarNodeId).lowerBound());
    maxVal =
        std::max(maxVal, _invariantGraph->varNode(inputVarNodeId).upperBound());
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(_invariantGraph->varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = _invariantGraph->varNode(outputVarNodeId).upperBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_LE(minVal, _invariantGraph->varNode(outputVarNodeId).lowerBound());
  EXPECT_GE(maxVal, _invariantGraph->varNode(outputVarNodeId).upperBound());
}

TEST_P(ArrayIntMinimumNodeTestFixture, replace) {
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

TEST_P(ArrayIntMinimumNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  if (shouldBeSubsumed()) {
    const Int expected = computeOutput(solver);
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
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

  std::vector<Int> inputVals = makeInputVals(solver, inputVarIds);

  while (increaseNextVal(solver, inputVarIds, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVarIds, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    expectVarVals(solver, inputVarIds, inputVals);

    const Int expected = computeOutput(solver);
    const Int actual = solver.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayIntMinimumNodeTest, ArrayIntMinimumNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
