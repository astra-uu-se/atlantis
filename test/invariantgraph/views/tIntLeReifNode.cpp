#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intLeReifNode.hpp"

class IntLeReifNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(r, 0, 1);

  fznparser::Constraint constraint{"int_le_reif", {"a", "b", "r"}, {}};

  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntLeReifNode> node;

  IntLeReifNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::IntLeReifNode>(constraint);
  }
};

TEST_F(IntLeReifNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(r));
}

TEST_F(IntLeReifNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 3);
}
