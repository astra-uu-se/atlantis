#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;
using ::testing::ContainerEq;

class BoolLinearNodeTest : public NodeTestBase<BoolLinearNode> {
 public:
  Int numInputs = 3;
  std::vector<VarNodeId> inputs{};
  std::vector<Int> coeffs{};
  VarNodeId output{NULL_NODE_ID};

  Int computeOutput(const std::vector<Int>& inputVals) {
    Int sum = 0;
    for (size_t i = 0; i < inputVals.size(); ++i) {
      sum += inputVals.at(i) == 0 ? coeffs.at(i) : 0;
    }
    return sum;
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs.reserve(numInputs);
    coeffs.reserve(numInputs);
    for (Int i = 0; i < numInputs; ++i) {
      inputs.push_back(retrieveBoolVarNode("input" + std::to_string(i)));
      coeffs.push_back((i + 1) * (i % 2 == 0 ? -1 : 1));
    }

    output = retrieveIntVarNode(-numInputs, numInputs, "output");

    createInvariantNode(std::vector<Int>(coeffs),
                        std::vector<VarNodeId>(inputs), output);
  }
};

TEST_F(BoolLinearNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_THAT(invNode().coeffs(), ContainerEq(coeffs));
  EXPECT_THAT(invNode().staticInputVarNodeIds(), ContainerEq(inputs));
  EXPECT_THAT(invNode().outputVarNodeIds(), std::vector<VarNodeId>{output});
}

TEST_F(BoolLinearNodeTest, application) {
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

TEST_F(BoolLinearNodeTest, propagation) {
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
