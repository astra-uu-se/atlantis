#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/linearNode.hpp"

class LinearNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;

  std::unique_ptr<invariantgraph::LinearNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 10);
    b = FZN_SEARCH_VARIABLE("b", 3, 7);
    c = FZN_VALUE(3);

    FZN_DEFINES_VAR_ANNOTATION(annotations, a);
    auto constraint =
        makeConstraint("int_lin_eq", annotations,
                       FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(1)),
                       FZN_VECTOR_CONSTRAINT_ARG(a, b), c);

    node = makeNode<invariantgraph::LinearNode>(constraint);
  }
};

TEST_F(LinearNodeTest, construction) {
  EXPECT_EQ(node->coeffs().size(), 1);
  EXPECT_EQ(node->coeffs()[0], 1);

  EXPECT_EQ(node->variables().size(), 1);
  EXPECT_EQ(node->variables()[0]->variable(), b);
  EXPECT_EQ(node->variables()[0]->inputFor().size(), 1);
  EXPECT_EQ(node->variables()[0]->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), a);
}

TEST_F(LinearNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(a)), 0);
  EXPECT_EQ(engine.upperBound(engineVariable(a)), 4);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b, intermediate (a is a view, and is not counted here)
  EXPECT_EQ(engine.numVariables(), 2);

  // linear
  EXPECT_EQ(engine.numInvariants(), 1);
}
