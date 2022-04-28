#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayBoolAndNode.hpp"

class ArrayBoolAndNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  fznparser::Constraint constraint{
      "array_bool_and",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, r.name},
      {}};
  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayBoolAndNode> node;

  ArrayBoolAndNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::ArrayBoolAndNode>(constraint);
  }
};

TEST_F(ArrayBoolAndNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(r));

  EXPECT_EQ(node->as().size(), 2);
  EXPECT_EQ(*node->as()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->as()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));

  expectMarkedAsInput(node.get(), node->as());
}

TEST_F(ArrayBoolAndNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 3);

  // sum
  EXPECT_EQ(engine.numInvariants(), 1);
}