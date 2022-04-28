#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/eqNode.hpp"

class EqNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  fznparser::Constraint constraint{"int_eq", {"a", "b"}, {}};
  fznparser::FZNModel model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::EqNode> node;

  EqNodeTest() : NodeTestBase(model) {}

  void SetUp() override { node = makeNode<invariantgraph::EqNode>(constraint); }
};

TEST_F(EqNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(EqNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  EXPECT_FALSE(_variableMap.contains(node->violation()));
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  EXPECT_TRUE(_variableMap.contains(node->violation()));
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and the violation
  EXPECT_EQ(engine.numVariables(), 3);

  // equal
  EXPECT_EQ(engine.numInvariants(), 1);

  EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
  EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 8);
}
