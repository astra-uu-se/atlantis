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
  std::vector<VarNodeId> inputs{};
  std::vector<Int> coeffs{};
  VarNodeId output{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int computeOutput(propagation::Solver& solver) {
    Int sum = 0;
    for (size_t i = 0; i < coeffs.size(); ++i) {
      if (varNode(inputs.at(i)).isFixed()) {
        sum += varNode(inputs.at(i)).lowerBound() * coeffs.at(i);
      } else {
        sum += solver.currentValue(varId(inputs.at(i))) * coeffs.at(i);
      }
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs.reserve(numInputs);
    coeffs.reserve(numInputs);
    Int minSum = 0;
    Int maxSum = 0;
    const Int lb = -2;
    const Int ub = 2;
    for (Int i = 0; i < static_cast<Int>(numInputs); ++i) {
      if (shouldBeSubsumed()) {
        const Int val = i % 2 == 0 ? lb : ub;
        inputs.emplace_back(
            retrieveIntVarNode(val, val, "input" + std::to_string(i)));
      } else {
        inputs.emplace_back(
            retrieveIntVarNode(lb, ub, "input" + std::to_string(i)));
      }
      coeffs.emplace_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min(lb * coeffs.back(), ub * coeffs.back());
      maxSum += std::max(lb * coeffs.back(), ub * coeffs.back());
    }

    output = retrieveIntVarNode(minSum, maxSum, outputIdentifier);

    createInvariantNode(std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputs), output);
  }
};

TEST_P(IntLinearNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputs));
  EXPECT_THAT(invNode().outputVarNodeIds(),
              ContainerEq(std::vector<VarNodeId>{output}));
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

  EXPECT_LE(solver.searchVars().size(), inputs.size());

  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    EXPECT_THAT(solver.searchVars(), Contains(varId(inputVarNodeId)));
  }
  EXPECT_LE(solver.numVars(), inputs.size() + 1);

  // linear invariant
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(IntLinearNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(output).isFixed());
    Int expected = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
      invNode().updateState(*_invariantGraph);
      expected += varNode(inputs.at(i)).lowerBound() * coeffs.at(i);
    }
    const Int actual = varNode(output).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(output).isFixed());
  }
}

TEST_P(IntLinearNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  VarNode& outputNode = varNode(outputIdentifier);
  if (outputNode.isFixed()) {
    Int expected = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
      expected += varNode(inputs.at(i)).lowerBound() * coeffs.at(i);
    }
    const Int actual = varNode(output).lowerBound();
    EXPECT_EQ(expected, actual);
    return;
  }

  std::vector<propagation::VarId> inputVars;
  for (const auto& nId : inputs) {
    if (!varNode(nId).isFixed()) {
      EXPECT_NE(varId(nId), propagation::NULL_ID);
      inputVars.emplace_back(varId(nId));
    }
  }

  EXPECT_FALSE(inputVars.empty());

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

INSTANTIATE_TEST_CASE_P(IntLinearNodeTest, IntLinearNodeTestFixture,
                        ::testing::Values(ParamData{InvariantNodeAction::NONE},
                                          ParamData{
                                              InvariantNodeAction::SUBSUME}));

}  // namespace atlantis::testing
