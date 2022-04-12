#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intLinLeNode.hpp"

class LeqNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  Int sum{3};

  fznparser::Constraint constraint{
      "int_lin_le",
      {fznparser::Constraint::ArrayArgument{1, 2},
       fznparser::Constraint::ArrayArgument{"a", "b"}, sum},
      {}};

  fznparser::FZNModel model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntLinLeNode> node;

  LeqNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::IntLinLeNode>(constraint);
  }
};

TEST_F(LeqNodeTest, construction) {
  EXPECT_EQ(*node->variables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->variables()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->coeffs()[0], 1);
  EXPECT_EQ(node->coeffs()[1], 2);
  EXPECT_EQ(node->bound(), 3);
  expectMarkedAsInput(node.get(), node->variables());
}

TEST_F(LeqNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, the bound (which we have to represent as a variable, but it has a
  // unit domain so a search wouldn't use it).
  EXPECT_EQ(engine.searchVariables().size(), 3);

  // a, b, the linear sum of a and b, the bound (we have to represent it as an
  // IntVar), the violation of the <= constraint.
  EXPECT_EQ(engine.numVariables(), 5);

  // linear and <=
  EXPECT_EQ(engine.numInvariants(), 2);
}
