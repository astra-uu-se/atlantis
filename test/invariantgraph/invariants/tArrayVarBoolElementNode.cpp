#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarBoolElementNodeTest
    : public NodeTestBase<ArrayVarBoolElementNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId c;

  VarNodeId idx;
  VarNodeId y;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    c = createBoolVar("c");
    idx = createIntVar(0, 10, "idx");
    y = createBoolVar("y");

    fznparser::BoolVarArray inputs("");
    inputs.append(boolVar(a));
    inputs.append(boolVar(b));
    inputs.append(boolVar(c));

    _model->addConstraint(fznparser::Constraint(
        "array_var_bool_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(idx)}, inputs,
                                    fznparser::BoolArg{boolVar(y)}}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayVarBoolElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().b(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), y);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 3);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(0), a);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(1), b);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(2), c);
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

  // a, b, c, idx
  EXPECT_EQ(solver.searchVars().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(solver.numVars(), 5);

  // elementVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayVarBoolElementNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputs;
  inputs.emplace_back(varId(invNode().staticInputVarNodeIds().front()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputs.emplace_back(varId(varNodeId));
    solver.updateBounds(varId(varNodeId), 0, 10, true);
  }
  solver.close();
  std::vector<Int> values(4, 0);

  for (values.at(0) = std::max(Int(1), solver.lowerBound(inputs.at(0)));
       values.at(0) <= std::min(Int(3), solver.upperBound(inputs.at(0)));
       ++values.at(0)) {
    for (values.at(1) = 0; values.at(1) <= 1; ++values.at(1)) {
      for (values.at(2) = 0; values.at(2) <= 1; ++values.at(2)) {
        for (values.at(3) = 0; values.at(3) <= 1; ++values.at(3)) {
          solver.beginMove();
          for (size_t i = 0; i < inputs.size(); ++i) {
            solver.setValue(inputs.at(i), values.at(i));
          }
          solver.endMove();

          solver.beginProbe();
          solver.query(outputId);
          solver.endProbe();
          EXPECT_EQ(solver.currentValue(outputId), values.at(values.at(0)) > 0);
        }
      }
    }
  }
}
}  // namespace atlantis::testing