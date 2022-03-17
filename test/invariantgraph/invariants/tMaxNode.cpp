#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/maxNode.hpp"

class MaxNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;

  std::unique_ptr<invariantgraph::MaxNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 0, 20);
    c = FZN_SEARCH_VARIABLE("c", 0, 10);

    FZN_DEFINES_VAR_ANNOTATION(annotations, c);
    auto constraint = makeConstraint("array_int_maximum", annotations, c,
                                     FZN_VECTOR_CONSTRAINT_ARG(a, b));

    node = makeNode<invariantgraph::MaxNode>(constraint);
  }
};

TEST_F(MaxNodeTest, construction) {
  EXPECT_EQ(node->variables().size(), 2);
  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), c);
  expectMarkedAsInput(node.get(), node->variables());
}

TEST_F(MaxNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(c)), 5);
  EXPECT_EQ(engine.upperBound(engineVariable(c)), 20);

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // maxSparse
  EXPECT_EQ(engine.numInvariants(), 1);
}
