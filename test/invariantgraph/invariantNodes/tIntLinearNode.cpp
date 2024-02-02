#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"
#include "propagation/solver.hpp"

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
      inputs.push_back(createIntVarNode(lb, ub, "input" + std::to_string(i)));
      coeffs.push_back((i + 1) * (i % 2 == 0 ? -1 : 1));
      minSum += std::min(lb * coeffs.back(), ub * coeffs.back());
      maxSum += std::max(lb * coeffs.back(), ub * coeffs.back());
    }

    output = createIntVarNode(minSum, maxSum, "output", true);

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