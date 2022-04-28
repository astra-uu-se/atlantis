#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intPowNode.hpp"

class IntPowNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  INT_VARIABLE(c, 0, 10);

  fznparser::Constraint constraint{
      "int_pow", {"a", "b", "c"}, {fznparser::DefinesVariableAnnotation{"c"}}};

  fznparser::FZNModel model{{}, {a, b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntPowNode> node;

  IntPowNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = invariantgraph::BinaryOpNode::fromModelConstraint<
        invariantgraph::IntPowNode>(_model, constraint, nodeFactory);
  }
};

TEST_F(IntPowNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntPowNodeTest, application) {
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

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // intPow
  EXPECT_EQ(engine.numInvariants(), 1);
}
