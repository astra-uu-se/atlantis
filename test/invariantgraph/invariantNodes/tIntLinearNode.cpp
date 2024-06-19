#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;

class IntLinearNodeTest : public NodeTestBase<IntLinearNode> {
 public:
  Int numInputs = 3;
  std::vector<VarNodeId> inputs{};
  std::vector<Int> coeffs{};
  VarNodeId output{NULL_NODE_ID};

  Int computeOutput(const std::vector<Int>& inputVals) {
    Int sum = 0;
    for (size_t i = 0; i < inputVals.size(); ++i) {
      sum += inputVals.at(i) * coeffs.at(i);
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
    for (Int i = 0; i < numInputs; ++i) {
      inputs.push_back(retrieveIntVarNode(lb, ub, "input" + std::to_string(i)));
      coeffs.push_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min(lb * coeffs.back(), ub * coeffs.back());
      maxSum += std::max(lb * coeffs.back(), ub * coeffs.back());
    }

    output = retrieveIntVarNode(minSum, maxSum, "output");

    createInvariantNode(std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputs), output);
  }
};

TEST_F(IntLinearNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputs));
  EXPECT_THAT(invNode().outputVarNodeIds(),
              ContainerEq(std::vector<VarNodeId>{output}));
}

TEST_F(IntLinearNodeTest, application) {
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

  // inputs and output
  EXPECT_EQ(solver.searchVars().size(), numInputs);

  // inputs and output
  EXPECT_EQ(solver.numVars(), numInputs + 1);

  // linear invariant
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(IntLinearNodeTest, updateState) {
  for (const auto& input : inputs) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    _invariantGraph->varNode(input).fixToValue(Int{0});
    invNode().updateState(*_invariantGraph);
  }
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  EXPECT_TRUE(_invariantGraph->varNode(output).isFixed());
}

TEST_F(IntLinearNodeTest, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  for (Int i = 0; i < numInputs - 1; ++i) {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
    _invariantGraph->varNode(inputs.at(i)).fixToValue(Int{0});
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  }
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(IntLinearNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), numInputs);
  std::vector<propagation::VarId> inputVars;
  inputVars.reserve(numInputs);
  for (const auto& varNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(varNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(varNodeId));
  }

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());

  solver.close();
  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int expected = computeOutput(inputVals);

    EXPECT_EQ(actual, expected);
  }
}
}  // namespace atlantis::testing
