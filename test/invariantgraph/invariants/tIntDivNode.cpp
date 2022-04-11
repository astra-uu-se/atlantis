#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intDivNode.hpp"

class IntDivNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 1, 10);
  INT_VARIABLE(c, 3, 5);

  fznparser::Constraint constraint{
      "int_div", {"a", "b", "c"}, {fznparser::DefinesVariableAnnotation{"c"}}};

  fznparser::FZNModel model{{}, {a, b, c}, {constraint}, fznparser::Satisfy{}};

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;
  std::unique_ptr<invariantgraph::IntDivNode> node;

  IntDivNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = invariantgraph::BinaryOpNode::fromModelConstraint<
        invariantgraph::IntDivNode>(_model, constraint, nodeFactory);
  }
};

TEST_F(IntDivNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntDivNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // intDiv
  EXPECT_EQ(engine.numInvariants(), 1);
}
