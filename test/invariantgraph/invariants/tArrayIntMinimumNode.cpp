#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntMinimumTestNode : public NodeTestBase<ArrayIntMinimumNode> {
 public:
  VarNodeId x1;
  VarNodeId x2;
  VarNodeId x3;
  VarNodeId o;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(5, 10, "x1");
    x2 = createIntVar(0, 20, "x2");
    x3 = createIntVar(0, 20, "x3");
    o = createIntVar(-10, 100, "o");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(x1));
    inputs.append(intVar(x2));
    inputs.append(intVar(x3));

    _model->addConstraint(fznparser::Constraint(
        "array_int_minimum",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(o)}, inputs},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(o))}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayIntMinimumTestNode, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), x1);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(1), x2);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(2), x3);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), o);
}

TEST_F(ArrayIntMinimumTestNode, application) {
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

  EXPECT_EQ(solver.lowerBound(varId(o)), 0);
  EXPECT_EQ(solver.upperBound(varId(o)), 10);

  // x1, x2, and x3
  EXPECT_EQ(solver.searchVariables().size(), 3);

  // x1, x2 and o
  EXPECT_EQ(solver.numVariables(), 4);

  // maxSparse
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayIntMinimumTestNode, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVariables(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputs;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputs.size(), 3);

  std::vector<Int> values(inputs.size());
  solver.close();

  for (values.at(0) = solver.lowerBound(inputs.at(0));
       values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = solver.lowerBound(inputs.at(1));
         values.at(2) <= solver.upperBound(inputs.at(2)); ++values.at(2)) {
      for (values.at(2) = solver.lowerBound(inputs.at(2));
           values.at(2) <= solver.upperBound(inputs.at(2)); ++values.at(2)) {
        solver.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          solver.setValue(inputs.at(i), values.at(i));
        }
        solver.endMove();

        solver.beginProbe();
        solver.query(outputId);
        solver.endProbe();

        const Int expected = *std::min_element(values.begin(), values.end());
        const Int actual = solver.currentValue(outputId);
        EXPECT_EQ(expected, actual);
      }
    }
  }
}

}  // namespace atlantis::testing