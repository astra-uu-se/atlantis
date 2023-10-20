#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/intLinearNode.hpp"

class LinearNodeTest : public NodeTestBase<invariantgraph::IntLinearNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId c;
  Int d{3};

  void makeView() {
    fznparser::IntVarArray coeffs("");
    coeffs.append(1);
    coeffs.append(1);

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(a));
    inputs.append(intVar(b));

    _model->addConstraint(fznparser::Constraint(
        "int_lin_eq",
        std::vector<fznparser::Arg>{coeffs, inputs, fznparser::IntArg{d}},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(a))}));

    makeInvNode(_model->constraints().front());
  }

  void makeLinear() {
    fznparser::IntVarArray coeffs("");
    coeffs.append(1);
    coeffs.append(1);
    coeffs.append(1);

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(a));
    inputs.append(intVar(b));
    inputs.append(intVar(c));

    _model->addConstraint(fznparser::Constraint(
        "int_lin_eq",
        std::vector<fznparser::Arg>{coeffs, inputs, fznparser::IntArg{d}},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(a))}));

    makeInvNode(_model->constraints().front());
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(0, 10, "a");
    b = createIntVar(3, 7, "b");
    c = createIntVar(3, 7, "c");
  }
};

TEST_F(LinearNodeTest, construction_should_register_view) {
  makeView();
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().coeffs().size(), 1);
  EXPECT_EQ(invNode().coeffs().at(0), 1);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), b);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().at(0), a);
}

TEST_F(LinearNodeTest, construction_should_register_linear) {
  makeLinear();
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().coeffs().size(), 2);
  EXPECT_EQ(invNode().coeffs().at(0), 1);
  EXPECT_EQ(invNode().coeffs().at(1), 1);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), b);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().at(0), a);
}

TEST_F(LinearNodeTest, application_should_register_view) {
  makeView();

  PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerOutputVariables(*_invariantGraph, engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, engine);
  engine.close();

  EXPECT_EQ(engine.lowerBound(varId(a)), -4);
  EXPECT_EQ(engine.upperBound(varId(a)), 0);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b
  EXPECT_EQ(engine.numVariables(), 1);
}

TEST_F(LinearNodeTest, application_should_register_linear) {
  makeLinear();
  PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerOutputVariables(*_invariantGraph, engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, engine);
  engine.close();

  EXPECT_EQ(engine.lowerBound(varId(a)), -11);
  EXPECT_EQ(engine.upperBound(varId(a)), -3);

  // b and c
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // b, c, intermediate (a is a view, and is not counted here)
  EXPECT_EQ(engine.numVariables(), 3);

  // linear
  EXPECT_EQ(engine.numInvariants(), 1);
}