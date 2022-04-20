#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"

class ArrayVarIntElementNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 3, 10);
  INT_VARIABLE(b, 2, 11);
  INT_VARIABLE(c, 1, 9);

  INT_VARIABLE(idx, 0, 10);
  INT_VARIABLE(y, 0, 10);

  fznparser::Constraint constraint{
      "array_var_int_element",
      {"idx", fznparser::Constraint::ArrayArgument{"a", "b", "c"}, "y"},
      {}};

  fznparser::FZNModel model{
      {}, {a, b, c, idx, y}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayVarIntElementNode> node;

  ArrayVarIntElementNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::ArrayVarIntElementNode>(constraint);
  }
};

TEST_F(ArrayVarIntElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(idx));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(y));
  expectMarkedAsInput(node.get(), {node->as()});
  expectMarkedAsInput(node.get(), {node->b()});

  EXPECT_EQ(node->as().size(), 3);
  EXPECT_EQ(node->as()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(node->as()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->as()[2]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
}

TEST_F(ArrayVarIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, idx.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c, idx
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(engine.numVariables(), 5);

  // elementVar
  EXPECT_EQ(engine.numInvariants(), 1);
}