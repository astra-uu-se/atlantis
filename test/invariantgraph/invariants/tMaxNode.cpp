#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/maxNode.hpp"

class MaxNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 0, 20);
  INT_VARIABLE(c, 0, 10);

  fznparser::Constraint constraint{
      "array_int_maximum",
      {"c", fznparser::Constraint::ArrayArgument{"a", "b"}},
      {fznparser::DefinesVariableAnnotation{"c"}}};

  fznparser::FZNModel model{{}, {a, b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::MaxNode> node;

  MaxNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::MaxNode>(constraint);
  }
};

TEST_F(MaxNodeTest, construction) {
  EXPECT_EQ(node->variables().size(), 2);
  EXPECT_EQ(*node->variables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->variables()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), node->variables());
}

TEST_F(MaxNodeTest, application) {
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

  EXPECT_EQ(engine.lowerBound(engineVariable(c)), 5);
  EXPECT_EQ(engine.upperBound(engineVariable(c)), 20);

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // maxSparse
  EXPECT_EQ(engine.numInvariants(), 1);
}
