#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntTimesNodeTestFixture : public NodeTestBase<IntTimesNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput() {
    Int product = 1;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      product *= varNode(inputVarNodeId).lowerBound();
    }
    return product;
  }

  Int computeOutput(propagation::Solver& solver) {
    Int product = 1;
    for (const auto& inputVarNodeId : inputVarNodeIds) {
      if (varNode(inputVarNodeId).isFixed()) {
        product *= varNode(inputVarNodeId).lowerBound();
      } else {
        product *= solver.currentValue(varId(inputVarNodeId));
      }
    }
    return product;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      if (_paramData.data == 0) {
        inputVarNodeIds = {retrieveIntVarNode(0, 0, "input_a"),
                           retrieveIntVarNode(0, 10, "input_b")};
      } else {
        inputVarNodeIds = {retrieveIntVarNode(-2, -2, "input_a"),
                           retrieveIntVarNode(2, 2, "input_b")};
      }
    } else if (shouldBeReplaced()) {
      inputVarNodeIds = {retrieveIntVarNode(1, 1, "input_a"),
                         retrieveIntVarNode(-2, 2, "input_b")};
    } else {
      inputVarNodeIds = {retrieveIntVarNode(-2, 2, "input_a"),
                         retrieveIntVarNode(-2, 2, "input_b")};
    }
    outputVarNodeId = retrieveIntVarNode(-10, 10, outputIdentifier);

    createInvariantNode(inputVarNodeIds.front(), inputVarNodeIds.back(),
                        outputVarNodeId);
  }
};

TEST_P(IntTimesNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
}

TEST_P(IntTimesNodeTestFixture, application) {
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

  // a and b
  EXPECT_EQ(solver.searchVars().size(), 2);

  // a, b and outputVarNodeId
  EXPECT_EQ(solver.numVars(), 3);

  // intTimes
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntTimesNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
  }
}

TEST_P(IntTimesNodeTestFixture, replace) {
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

TEST_P(IntTimesNodeTestFixture, propagation) {
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

  if (shouldBeSubsumed()) {
    VarNode& outputNode = varNode(outputVarNodeId);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
    return;
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

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    IntTimesNodeTest, IntTimesNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::SUBSUME, 1},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
