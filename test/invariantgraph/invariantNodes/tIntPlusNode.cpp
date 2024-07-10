#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::Contains;

class IntPlusNodeTestFixture : public NodeTestBase<IntPlusNode> {
 public:
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<std::string> inputIdentifiers{"input_1", "input_2"};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput() {
    Int sum = 0;
    for (const auto& identifier : inputIdentifiers) {
      EXPECT_TRUE(varNode(identifier).isFixed());
      sum += varNode(identifier).lowerBound();
    }
    return sum;
  }

  Int computeOutput(propagation::Solver& solver) {
    Int sum = 0;
    for (const auto& identifier : inputIdentifiers) {
      if (varNode(identifier).isFixed()) {
        sum += varNode(identifier).lowerBound();
      } else {
        sum += solver.currentValue(varId(identifier));
      }
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed()) {
      inputVarNodeIds = std::vector<VarNodeId>{
          retrieveIntVarNode(1, 1, inputIdentifiers.front()),
          retrieveIntVarNode(1, 1, inputIdentifiers.back())};
    } else if (shouldBeReplaced()) {
      inputVarNodeIds = std::vector<VarNodeId>{
          retrieveIntVarNode(0, 0, inputIdentifiers.front()),
          retrieveIntVarNode(-2, 2, inputIdentifiers.back())};
    } else {
      inputVarNodeIds = std::vector<VarNodeId>{
          retrieveIntVarNode(-2, 2, inputIdentifiers.front()),
          retrieveIntVarNode(-2, 2, inputIdentifiers.back())};
    }
    outputVarNodeId = retrieveIntVarNode(0, 10, outputIdentifier);

    createInvariantNode(inputVarNodeIds.front(), inputVarNodeIds.back(),
                        outputVarNodeId);
  }
};

TEST_P(IntPlusNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), inputVarNodeIds.size());
  for (size_t i = 0; i < inputVarNodeIds.size(); ++i) {
    EXPECT_EQ(invNode().staticInputVarNodeIds().at(i), inputVarNodeIds.at(i));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);
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
  for (const auto& identifier : inputIdentifiers) {
    const VarNode& inputNode = varNode(identifier);
    if (shouldBeSubsumed()) {
      EXPECT_TRUE(inputNode.isFixed());
    } else if (!shouldBeReplaced()) {
      EXPECT_FALSE(inputNode.isFixed());
    }
    if (!inputNode.isFixed()) {
      EXPECT_THAT(solver.searchVars(), Contains(varId(identifier)));
    }
  }
  EXPECT_LE(solver.searchVars().size(), 2);

  if (!shouldBeSubsumed()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }

  // a, b and outputVarNodeId
  EXPECT_LE(solver.numVars(), 3);

  // intPow
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntPlusNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVarNodeId).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVarNodeId).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVarNodeId).isFixed());
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

  if (shouldBeSubsumed()) {
    VarNode& outputNode = varNode(outputIdentifier);
    EXPECT_TRUE(outputNode.isFixed());
    const Int actual = outputNode.lowerBound();
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
    return;
  }

  if (shouldBeReplaced()) {
    EXPECT_FALSE(varNode(outputIdentifier).isFixed());
    const VarNodeId addantVarNodeId =
        varNode(varNode(inputIdentifiers.front()).isFixed()
                    ? inputIdentifiers.back()
                    : inputIdentifiers.front())
            .varNodeId();
    EXPECT_EQ(varNode(outputIdentifier).varNodeId(), addantVarNodeId);
    return;
  }

  std::vector<propagation::VarId> inputVarIds;
  for (const auto& identifiers : inputIdentifiers) {
    if (!varNode(identifiers).isFixed()) {
      EXPECT_NE(varId(identifiers), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(identifiers));
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

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(solver);
    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    IntPlusNodeTest, IntPlusNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME},
                      ParamData{InvariantNodeAction::REPLACE}));

}  // namespace atlantis::testing
