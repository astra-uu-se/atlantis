#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;
using ::testing::Contains;

class IntLinearNodeTestFixture : public NodeTestBase<IntLinearNode> {
 public:
  size_t numInputs = 3;
  std::vector<VarNodeId> inputVarNodeIds;
  std::vector<Int> coeffs;
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput() {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      EXPECT_TRUE(varNode(inputVarNodeIds.at(i)).isFixed());
      sum += varNode(inputVarNodeIds.at(i)).lowerBound() * coeffs.at(i);
    }
    return sum;
  }

  Int computeOutput(propagation::Solver& solver) {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (coeffs.at(i) == 0) {
        continue;
      }
      if (varNode(inputVarNodeIds.at(i)).isFixed()) {
        sum += varNode(inputVarNodeIds.at(i)).lowerBound() * coeffs.at(i);
      } else {
        sum += solver.currentValue(varId(inputVarNodeIds.at(i))) * coeffs.at(i);
      }
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputVarNodeIds.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    const Int lb = -2;
    const Int ub = 2;
    for (Int i = 0; i < static_cast<Int>(numInputs); ++i) {
      if (shouldBeSubsumed()) {
        const Int val = i % 2 == 0 ? lb : ub;
        inputVarNodeIds.emplace_back(
            retrieveIntVarNode(val, val, "input" + std::to_string(i)));
      } else {
        inputVarNodeIds.emplace_back(
            retrieveIntVarNode(lb, ub, "input" + std::to_string(i)));
      }
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min(lb * coeffs.back(), ub * coeffs.back());
      maxSum += std::max(lb * coeffs.back(), ub * coeffs.back());
    }

    outputVarNodeId = retrieveIntVarNode(minSum, maxSum, outputIdentifier);

    createInvariantNode(std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputVarNodeIds),
                        outputVarNodeId);
  }
};

TEST_P(IntLinearNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputVarNodeIds));
  EXPECT_THAT(invNode().outputVarNodeIds(),
              ContainerEq(std::vector<VarNodeId>{outputVarNodeId}));
}

TEST_P(IntLinearNodeTestFixture, application) {
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

  EXPECT_LE(solver.searchVars().size(), inputVarNodeIds.size());

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    EXPECT_THAT(solver.searchVars(), Contains(varId(inputVarNodeId)));
  }
  EXPECT_LE(solver.numVars(), inputVarNodeIds.size() + 1);

  // linear invariant
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntLinearNodeTestFixture, updateState) {
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

TEST_P(IntLinearNodeTestFixture, propagation) {
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
  for (const auto& inputVarNodeId : inputVarNodeIds) {
    if (!varNode(inputVarNodeId).isFixed()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(inputVarNodeId));
    }
  }

  EXPECT_FALSE(inputVarIds.empty());

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
    IntLinearNodeTest, IntLinearNodeTestFixture,
    ::testing::Values(ParamData{}, ParamData{InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
