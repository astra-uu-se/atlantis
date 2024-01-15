#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntElementNodeTest : public NodeTestBase<ArrayIntElementNode> {
 public:
  VarNodeId idx = NULL_NODE_ID;
  VarNodeId outputVar = NULL_NODE_ID;
  std::vector<Int> elementValues{1, 2, 3};

  void SetUp() override {
    NodeTestBase::SetUp();
    addFznVar(0, 10, "idx");
    addFznVar(0, 10, "outputVar");

    fznparser::IntVarArray elements("");
    for (const auto& elem : elementValues) {
      elements.append(elem);
    }

    _model->addConstraint(fznparser::Constraint(
        "array_int_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar("idx")}, elements,
                                    fznparser::IntArg{intVar("outputVar")}}));

    fzn::array_int_element(*_invariantGraph, _model->constraints.front());

    idx = varNodeId("idx");
    outputVar = varNodeId("outputVar");
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVar);

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

  // idx (outputVar is a view)
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

  const propagation::VarId input = inputs.front();
  solver.close();

  for (Int value = solver.lowerBound(input); value <= solver.upperBound(input);
       ++value) {
    solver.beginMove();
    solver.setValue(input, value);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    if (0 < value && value <= static_cast<Int>(elementValues.size())) {
      EXPECT_EQ(solver.currentValue(outputId), elementValues.at(value - 1));
    }
  }
}

}  // namespace atlantis::testing