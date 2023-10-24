#include "../nodeTestBase.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class Bool2IntNodeTest : public NodeTestBase<Bool2IntNode> {
 public:
  VarNodeId a;
  VarNodeId b;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createIntVar(0, 1, "b");

    _model->addConstraint(fznparser::Constraint(
        "bool2int",
        std::vector<fznparser::Arg>{fznparser::BoolArg{boolVar(a)},
                                    fznparser::IntArg{intVar(b)}},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(b))}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(Bool2IntNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), a);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), b);
}

TEST_F(Bool2IntNodeTest, application) {
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

  // a
  EXPECT_EQ(solver.searchVars().size(), 1);

  // a
  EXPECT_EQ(solver.numVars(), 1);
}

TEST_F(Bool2IntNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), propagation::NULL_ID);
  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);

  const propagation::VarId input = varId(invNode().staticInputVarNodeIds().front());
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());

  for (Int value = solver.lowerBound(input); value <= solver.upperBound(input);
       ++value) {
    solver.beginMove();
    solver.setValue(input, value);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int expected = (value == 0);
    const Int actual = solver.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}

}  // namespace atlantis::testing