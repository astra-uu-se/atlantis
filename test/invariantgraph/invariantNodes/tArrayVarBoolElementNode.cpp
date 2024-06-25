#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_var_bool_element.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarBoolElementNodeTest : public NodeTestBase<ArrayVarElementNode> {
 public:
  std::vector<VarNodeId> inputs;

  VarNodeId idx{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  Int offsetIdx = 1;

  void SetUp() override {
    NodeTestBase::SetUp();
    inputs = {retrieveBoolVarNode("x1"), retrieveBoolVarNode("x2"),
              retrieveBoolVarNode("x3")};

    idx = retrieveIntVarNode(
        offsetIdx, static_cast<Int>(inputs.size()) - 1 + offsetIdx, "idx");
    output = retrieveBoolVarNode("output");

    createInvariantNode(idx, std::vector<VarNodeId>{inputs}, output, offsetIdx);
  }
};

TEST_F(ArrayVarBoolElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 3);
  for (size_t i = 0; i < inputs.size(); ++i) {
    EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(i), inputs.at(i));
  }
}

TEST_F(ArrayVarBoolElementNodeTest, application) {
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

  // x1, x2, x3, idx
  EXPECT_EQ(solver.searchVars().size(), 4);

  // x1, x2, x3, idx, output
  EXPECT_EQ(solver.numVars(), 5);

  // elementVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayVarBoolElementNodeTest, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  _invariantGraph->varNode(idx).fixToValue(Int{1});
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(ArrayVarBoolElementNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()),
            propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), inputs.size());
  for (const auto& inputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputVars;
  inputVars.emplace_back(varId(invNode().staticInputVarNodeIds().front()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputVars.emplace_back(varId(varNodeId));
    solver.updateBounds(varId(varNodeId), 0, 10, true);
  }
  solver.close();
  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actualIndex = solver.currentValue(inputVars.at(0)) - offsetIdx;
    EXPECT_GE(actualIndex, 0);
    EXPECT_LT(actualIndex, inputVals.size());

    const Int expectedIndex = inputVals.at(0) - offsetIdx;
    EXPECT_EQ(actualIndex, expectedIndex);

    const Int actual = solver.currentValue(outputId);
    // +1 as the first index is the index variable:
    const Int expected = inputVals.at(actualIndex + 1);

    EXPECT_EQ(actual == 0, expected == 0);
  }
}

}  // namespace atlantis::testing
