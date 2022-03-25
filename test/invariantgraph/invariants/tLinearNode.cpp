#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/linearNode.hpp"

class LinearNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;
  std::shared_ptr<fznparser::ValueLiteral> d;

  std::shared_ptr<fznparser::Constraint> c1;
  std::shared_ptr<fznparser::Constraint> c2;

  std::unique_ptr<invariantgraph::LinearNode> shouldRegisterView;
  std::unique_ptr<invariantgraph::LinearNode> shouldRegisterLinear;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 10);
    b = FZN_SEARCH_VARIABLE("b", 3, 7);
    c = FZN_SEARCH_VARIABLE("c", 3, 7);
    d = FZN_VALUE(3);

    FZN_DEFINES_VAR_ANNOTATION(a1, a);
    c1 = makeConstraint("int_lin_eq", a1,
                        FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(1)),
                        FZN_VECTOR_CONSTRAINT_ARG(a, b), d);

    FZN_DEFINES_VAR_ANNOTATION(a2, a);
    c2 = makeConstraint(
        "int_lin_eq", a2,
        FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(1), FZN_VALUE(1)),
        FZN_VECTOR_CONSTRAINT_ARG(a, b, c), d);
  }
};

TEST_F(LinearNodeTest, construction_should_register_view) {
  shouldRegisterView = makeNode<invariantgraph::LinearNode>(c1);

  EXPECT_EQ(shouldRegisterView->coeffs().size(), 1);
  EXPECT_EQ(shouldRegisterView->coeffs()[0], 1);

  EXPECT_EQ(shouldRegisterView->variables().size(), 1);
  EXPECT_EQ(shouldRegisterView->variables()[0]->variable(), b);
  EXPECT_EQ(shouldRegisterView->variables()[0]->inputFor().size(), 1);
  EXPECT_EQ(shouldRegisterView->variables()[0]->inputFor()[0],
            shouldRegisterView.get());

  EXPECT_EQ(shouldRegisterView->definedVariables().size(), 1);
  EXPECT_EQ(shouldRegisterView->definedVariables()[0]->variable(), a);
}

TEST_F(LinearNodeTest, construction_should_register_linear) {
  shouldRegisterLinear = makeNode<invariantgraph::LinearNode>(c2);

  EXPECT_EQ(shouldRegisterLinear->coeffs().size(), 2);
  EXPECT_EQ(shouldRegisterLinear->coeffs()[0], 1);
  EXPECT_EQ(shouldRegisterLinear->coeffs()[1], 1);

  EXPECT_EQ(shouldRegisterLinear->variables().size(), 2);
  EXPECT_EQ(shouldRegisterLinear->variables()[0]->variable(), b);
  EXPECT_EQ(shouldRegisterLinear->variables()[0]->inputFor().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->variables()[0]->inputFor()[0],
            shouldRegisterLinear.get());

  EXPECT_EQ(shouldRegisterLinear->variables()[1]->variable(), c);
  EXPECT_EQ(shouldRegisterLinear->variables()[1]->inputFor().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->variables()[1]->inputFor()[0],
            shouldRegisterLinear.get());

  EXPECT_EQ(shouldRegisterLinear->definedVariables().size(), 1);
  EXPECT_EQ(shouldRegisterLinear->definedVariables()[0]->variable(), a);
}

TEST_F(LinearNodeTest, application_should_register_view) {
  shouldRegisterView = makeNode<invariantgraph::LinearNode>(c1);

  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b});
  shouldRegisterView->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(a)), 0);
  EXPECT_EQ(engine.upperBound(engineVariable(a)), 4);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b
  EXPECT_EQ(engine.numVariables(), 1);
}

TEST_F(LinearNodeTest, application_should_register_linear) {
  shouldRegisterLinear = makeNode<invariantgraph::LinearNode>(c2);
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b, c});
  shouldRegisterLinear->registerWithEngine(engine, _variableMap);
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
