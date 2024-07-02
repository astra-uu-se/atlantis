#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::Contains;

class IntPlusNodeTestFixture : public NodeTestBase<IntPlusNode> {
 public:
  std::vector<VarNodeId> inputs;
  VarNodeId output{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(propagation::Solver& solver) {
    Int sum = 0;
    for (const auto& input : inputs) {
      if (varNode(input).isFixed()) {
        sum += varNode(input).lowerBound();
      } else {
        sum += solver.currentValue(varId(input));
      }
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      inputs = std::vector<VarNodeId>{retrieveIntVarNode(1, 1, "a"),
                                      retrieveIntVarNode(1, 1, "b")};
    } else if (shouldBeReplaced()) {
      inputs = std::vector<VarNodeId>{retrieveIntVarNode(0, 0, "a"),
                                      retrieveIntVarNode(-2, 2, "b")};
    } else {
      inputs = std::vector<VarNodeId>{retrieveIntVarNode(-2, 2, "a"),
                                      retrieveIntVarNode(-2, 2, "b")};
    }
    output = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(inputs.front(), inputs.back(), output);
  }
};

TEST_P(IntPlusNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputs.size());
  for (size_t i = 0; i < inputs.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputs.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_P(IntPlusNodeTestFixture, application) {
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
  for (const auto& inputVarNodeId : inputs) {
    const VarNode& inputNode = varNode(inputVarNodeId);
    if (shouldBeSubsumed()) {
      EXPECT_TRUE(inputNode.isFixed());
    } else if (!shouldBeReplaced()) {
      EXPECT_FALSE(inputNode.isFixed());
    }
    if (!inputNode.isFixed()) {
      EXPECT_THAT(solver.searchVars(), Contains(varId(inputVarNodeId)));
    }
  }
  EXPECT_LE(solver.searchVars().size(), 2);

  if (!shouldBeSubsumed()) {
    EXPECT_NE(varId(output), propagation::NULL_ID);
  }

  // a, b and output
  EXPECT_LE(solver.numVars(), 3);

  // intPow
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntPlusNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(output).isFixed());
    Int expected = 0;
    for (const auto& input : inputs) {
      invNode().updateState(*_invariantGraph);
      expected += varNode(input).lowerBound();
    }
    const Int actual = varNode(output).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(output).isFixed());
  }
}

TEST_P(IntPlusNodeTestFixture, replace) {
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

TEST_P(IntPlusNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  std::vector<propagation::VarId> inputVars;
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  if (shouldBeSubsumed()) {
    VarNode& outputNode = varNode(output);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
    return;
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

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    IntPlusNodeTest, IntPlusNodeTestFixture,
    ::testing::Values(ParamData{InvariantNodeAction::NONE},
                      ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
