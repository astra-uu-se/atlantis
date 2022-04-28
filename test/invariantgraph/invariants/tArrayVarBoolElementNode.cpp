#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayVarBoolElementNode.hpp"

class ArrayVarBoolElementNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(c);

  INT_VARIABLE(idx, 0, 10);
  INT_VARIABLE(y, 0, 10);

  fznparser::Constraint constraint{
      "array_var_bool_element",
      {idx.name, fznparser::Constraint::ArrayArgument{a.name, b.name, c.name},
       y.name},
      {}};

  fznparser::FZNModel model{
      {}, {a, b, c, idx, y}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayVarBoolElementNode> node;

  ArrayVarBoolElementNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::ArrayVarBoolElementNode>(constraint);
  }
};

TEST_F(ArrayVarBoolElementNodeTest, construction) {
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

TEST_F(ArrayVarBoolElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, idx.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c, idx
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(engine.numVariables(), 5);

  // elementVar
  EXPECT_EQ(engine.numInvariants(), 1);
}