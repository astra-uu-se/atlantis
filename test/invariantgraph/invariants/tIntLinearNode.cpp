#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intLinearNode.hpp"

class LinearNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 3, 7);
  INT_VARIABLE(c, 3, 7);
  Int d{3};

  fznparser::Constraint c1{"int_lin_eq",
                           {fznparser::Constraint::ArrayArgument{1, 1},
                            fznparser::Constraint::ArrayArgument{"a", "b"}, d},
                           {fznparser::DefinesVariableAnnotation{"a"}}};

  fznparser::Constraint c2{
      "int_lin_eq",
      {fznparser::Constraint::ArrayArgument{1, 1, 1},
       fznparser::Constraint::ArrayArgument{"a", "b", "c"}, d},
      {fznparser::DefinesVariableAnnotation{"a"}}};

  fznparser::FZNModel model{{}, {a, b, c}, {c1, c2}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntLinearNode> shouldRegisterView;
  std::unique_ptr<invariantgraph::IntLinearNode> shouldRegisterLinear;

  void SetUp() override { setModel(&model); }
};

TEST_F(LinearNodeTest, construction_should_register_view) {
  shouldRegisterView = makeNode<invariantgraph::IntLinearNode>(c1);

  EXPECT_EQ(shouldRegisterView->coeffs().size(), 1);
  EXPECT_EQ(shouldRegisterView->coeffs()[0], 1);

  EXPECT_EQ(shouldRegisterView->staticInputs().size(), 1);
  EXPECT_EQ(shouldRegisterView->staticInputs()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(shouldRegisterView->staticInputs()[0]->inputFor().size(), 1);
  EXPECT_EQ(shouldRegisterView->staticInputs()[0]->inputFor()[0],
            shouldRegisterView.get());

  EXPECT_EQ(shouldRegisterView->definedVariables().size(), 1);
  EXPECT_EQ(shouldRegisterView->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
}

TEST_F(LinearNodeTest, construction_should_register_linear) {
  shouldRegisterLinear = makeNode<invariantgraph::IntLinearNode>(c2);

  EXPECT_EQ(shouldRegisterLinear->coeffs().size(), 2);
  EXPECT_EQ(shouldRegisterLinear->coeffs()[0], 1);
  EXPECT_EQ(shouldRegisterLinear->coeffs()[1], 1);

  EXPECT_EQ(shouldRegisterLinear->staticInputs().size(), 2);
  EXPECT_EQ(shouldRegisterLinear->staticInputs()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(shouldRegisterLinear->staticInputs()[0]->inputFor().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->staticInputs()[0]->inputFor()[0],
            shouldRegisterLinear.get());

  EXPECT_EQ(shouldRegisterLinear->staticInputs()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  EXPECT_EQ(shouldRegisterLinear->staticInputs()[1]->inputFor().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->staticInputs()[1]->inputFor()[0],
            shouldRegisterLinear.get());

  EXPECT_EQ(shouldRegisterLinear->definedVariables().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
}

TEST_F(LinearNodeTest, application_should_register_view) {
  shouldRegisterView = makeNode<invariantgraph::IntLinearNode>(c1);

  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b.name});
  for (auto* const definedVariable : shouldRegisterView->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterView->createDefinedVariables(engine);
  for (auto* const definedVariable : shouldRegisterView->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterView->registerWithEngine(engine);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(a)), 0);
  EXPECT_EQ(engine.upperBound(engineVariable(a)), 4);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b
  EXPECT_EQ(engine.numVariables(), 1);
}

TEST_F(LinearNodeTest, application_should_register_linear) {
  shouldRegisterLinear = makeNode<invariantgraph::IntLinearNode>(c2);
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b.name, c.name});
  for (auto* const definedVariable : shouldRegisterLinear->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterLinear->createDefinedVariables(engine);
  for (auto* const definedVariable : shouldRegisterLinear->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  shouldRegisterLinear->registerWithEngine(engine);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(a)), 3);
  EXPECT_EQ(engine.upperBound(engineVariable(a)), 11);

  // b and c
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // b, c, intermediate (a is a view, and is not counted here)
  EXPECT_EQ(engine.numVariables(), 3);

  // linear
  EXPECT_EQ(engine.numInvariants(), 1);
}