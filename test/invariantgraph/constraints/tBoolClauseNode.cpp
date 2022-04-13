#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/boolClauseNode.hpp"

class BoolClauseNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(c);
  BOOL_VARIABLE(d);

  fznparser::Constraint constraint{
      "bool_clause",
      {fznparser::Constraint::ArrayArgument{a.name, b.name},
       fznparser::Constraint::ArrayArgument{c.name, d.name}},
      {}};
  fznparser::FZNModel model{
      {}, {a, b, c, d}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::BoolClauseNode> node;

  BoolClauseNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::BoolClauseNode>(constraint);
  }
};

TEST_F(BoolClauseNodeTest, construction) {
  EXPECT_EQ(node->as().size(), 2);
  EXPECT_EQ(*node->as()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->as()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));

  EXPECT_EQ(node->bs().size(), 2);
  EXPECT_EQ(*node->bs()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  EXPECT_EQ(*node->bs()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(d));

  expectMarkedAsInput(node.get(), node->as());
  expectMarkedAsInput(node.get(), node->bs());
}

TEST_F(BoolClauseNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, d.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c and d, constZero
  EXPECT_EQ(engine.searchVariables().size(), 5);

  // a, b, c, d, sum, constZero and the violation
  EXPECT_EQ(engine.numVariables(), 7);

  // lessThan, linear
  EXPECT_EQ(engine.numInvariants(), 2);
}
