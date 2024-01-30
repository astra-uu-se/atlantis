#include "../nodeTestBase.hpp"
#include "invariantgraph/fzn/array_bool_element.hpp"
#include "invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayBoolElementNodeTest : public NodeTestBase<ArrayElementNode> {
 public:
  VarNodeId output{NULL_NODE_ID};
  VarNodeId idx{NULL_NODE_ID};

  Int offsetIdx = 1;

  std::vector<bool> elementValues{true, false, false, true};

  void SetUp() override {
    NodeTestBase::SetUp();
    idx = createIntVarNode(1, 4, "idx");
    output = createIntVarNode(1, 4, "output", true);

    createInvariantNode(std::vector<bool>(elementValues), idx,
                        output, offsetIdx);
  }
};

TEST_F(ArrayBoolElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  std::vector<Int> expectedAs{0, 1, 1, 0};
  EXPECT_EQ(invNode().as(), expectedAs);
}

TEST_F(ArrayBoolElementNodeTest, application) {
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

  // The index ranges over the as array (first index is 1).
  EXPECT_EQ(solver.lowerBound(varId(idx)), 1);
  EXPECT_EQ(solver.upperBound(varId(idx)), invNode().as().size());

  // The output domain should contain all elements in as.
  EXPECT_EQ(solver.lowerBound(varId(output)), 0);
  EXPECT_EQ(solver.upperBound(varId(output)), 1);

  // output
  EXPECT_EQ(solver.searchVars().size(), 1);

  // output (output is a view)
  EXPECT_EQ(solver.numVars(), 1);

  // elementConst is a view
  EXPECT_EQ(solver.numInvariants(), 0);
}

TEST_F(ArrayBoolElementNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputs;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputs.size(), 1);

  const propagation::VarId inputVar = inputs.front();
  solver.close();

  for (Int inputVal = solver.lowerBound(inputVar);
       inputVal <= solver.upperBound(inputVar); ++inputVal) {
    solver.beginMove();
    solver.setValue(inputVar, inputVal);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);

    EXPECT_EQ(actual == 0, elementValues.at(inputVal - offsetIdx));
  }
}

}  // namespace atlantis::testing