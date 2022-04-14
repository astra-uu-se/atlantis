#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"

class ArrayIntElementNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(b, 0, 10);
  INT_VARIABLE(c, 0, 10);

  fznparser::Constraint constraint{
      "array_int_element",
      {"b", fznparser::Constraint::ArrayArgument{1, 2, 3}, "c"},
      {}};
  fznparser::FZNModel model{{}, {b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayIntElementNode> node;

  ArrayIntElementNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::ArrayIntElementNode>(constraint);
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), {node->b()});

  std::vector<Int> expectedAs{1, 2, 3};
  EXPECT_EQ(node->as(), expectedAs);
}

TEST_F(ArrayIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b and c
  EXPECT_EQ(engine.numVariables(), 2);

  // elementConst
  EXPECT_EQ(engine.numInvariants(), 1);
}