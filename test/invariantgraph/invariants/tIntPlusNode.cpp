#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/intPlusNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntPlusNodeTest : public NodeTestBase<IntPlusNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId c;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(0, 10, "a");
    b = createIntVar(0, 10, "b");
    c = createIntVar(0, 10, "c");

    _model->addConstraint(fznparser::Constraint(
        "int_plus",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                    fznparser::IntArg{intVar(b)},
                                    fznparser::IntArg{intVar(c)}},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(c))}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(IntPlusNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().a(), a);
  EXPECT_EQ(invNode().b(), b);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), c);
}

TEST_F(IntPlusNodeTest, application) {
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

  // a and b
  EXPECT_EQ(solver.searchVars().size(), 2);

  // a, b and c
  EXPECT_EQ(solver.numVars(), 3);

  // intPow
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(IntPlusNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputs;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputs.size(), 2);

  std::vector<Int> values(inputs.size());
  solver.close();

  for (values.at(0) = solver.lowerBound(inputs.at(0));
       values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = solver.lowerBound(inputs.at(1));
         values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
      solver.beginMove();
      for (size_t i = 0; i < inputs.size(); ++i) {
        solver.setValue(inputs.at(i), values.at(i));
      }
      solver.endMove();

      solver.beginProbe();
      solver.query(outputId);
      solver.endProbe();

      EXPECT_EQ(solver.currentValue(outputId), values.at(0) + values.at(1));
    }
  }
}
}  // namespace atlantis::testing