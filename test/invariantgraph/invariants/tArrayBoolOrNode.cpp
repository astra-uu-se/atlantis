#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayBoolOrNode.hpp"

class ArrayBoolOrNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  fznparser::Constraint constraint{
      "array_bool_or",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, r.name},
      {}};
  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayBoolOrNode> node;

  ArrayBoolOrNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::ArrayBoolOrNode>(constraint);
  }
};

TEST_F(ArrayBoolOrNodeTest, construction) {
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

TEST_F(ArrayBoolOrNodeTest, application) {
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
  EXPECT_EQ(engine.searchVariables().size(), 3);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 5);

  // sum, less than
  EXPECT_EQ(engine.numInvariants(), 2);
}