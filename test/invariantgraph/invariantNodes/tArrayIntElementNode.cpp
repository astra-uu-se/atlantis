#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntElementNodeTest : public NodeTestBase<ArrayElementNode> {
 public:
  VarNodeId idx{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};
  std::vector<Int> elementValues{1, 2, 3};

  Int offsetIdx = 1;

  void SetUp() override {
    NodeTestBase::SetUp();
    idx = retrieveIntVarNode(
        offsetIdx, static_cast<Int>(elementValues.size()) - 1 + offsetIdx,
        "idx");
    output = retrieveIntVarNode(0, 10, "output");

    std::vector<Int> parVector(elementValues);

    createInvariantNode(std::move(parVector), idx, output, offsetIdx);
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  std::vector<Int> expectedAs{1, 2, 3};
  EXPECT_EQ(invNode().as(), expectedAs);
}

TEST_F(ArrayIntElementNodeTest, application) {
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

  // idx
  EXPECT_EQ(solver.searchVars().size(), 1);

  // idx (output is a view)
  EXPECT_EQ(solver.numVars(), 1);

  // elementConst is a view
  EXPECT_EQ(solver.numInvariants(), 0);
}

TEST_F(ArrayIntElementNodeTest, propagation) {
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

    const Int expected = solver.currentValue(outputId);
    const Int index = inputVal - offsetIdx;
    EXPECT_GE(index, 0);
    EXPECT_LT(index, elementValues.size());
    const Int actual = elementValues.at(index);

    EXPECT_EQ(expected, actual);
  }
}

}  // namespace atlantis::testing
