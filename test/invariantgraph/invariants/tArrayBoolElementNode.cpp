#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayBoolElementNodeTest : public NodeTestBase<ArrayIntElementNode> {
 public:
  VarNodeId b;
  VarNodeId y;
  std::vector<bool> elementValues{true, false, false, true};

  void SetUp() override {
    NodeTestBase::SetUp();
    b = createIntVar(1, 4, "b");
    y = createBoolVar("y");

    fznparser::BoolVarArray elements("");
    for (const auto& elem : elementValues) {
      elements.append(elem);
    }

    _model->addConstraint(fznparser::Constraint(
        "array_bool_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(b)}, elements,
                                    fznparser::BoolArg{boolVar(y)}}));

    makeOtherInvNode<ArrayBoolElementNode>(_model->constraints().front());
  }
};

TEST_F(ArrayBoolElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().b(), b);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), y);

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
  invNode().registerOutputVariables(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // The index ranges over the as array (first index is 1).
  EXPECT_EQ(solver.lowerBound(varId(b)), 1);
  EXPECT_EQ(solver.upperBound(varId(b)), invNode().as().size());

  // The output domain should contain all elements in as.
  EXPECT_EQ(solver.lowerBound(varId(y)), 0);
  EXPECT_EQ(solver.upperBound(varId(y)), 1);

  // b
  EXPECT_EQ(solver.searchVariables().size(), 1);

  // b (y is a view)
  EXPECT_EQ(solver.numVariables(), 1);

  // elementConst is a view
  EXPECT_EQ(solver.numInvariants(), 0);
}

TEST_F(ArrayBoolElementNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVariables(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputs;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());
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

    EXPECT_EQ(solver.currentValue(outputId), !elementValues.at(value - 1));
  }
}

}  // namespace atlantis::testing