#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/linEqNode.hpp"

class LinEqNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  Int sum{3};

  fznparser::Constraint constraint{
      "int_lin_eq",
      {fznparser::Constraint::ArrayArgument{1, 2},
       fznparser::Constraint::ArrayArgument{"a", "b"}, sum},
      {}};

  fznparser::FZNModel model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::LinEqNode> node;

  LinEqNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::LinEqNode>(constraint);
  }
};

TEST_F(LinEqNodeTest, construction) {
  EXPECT_EQ(*node->variables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->variables()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  expectMarkedAsInput(node.get(), node->variables());

  EXPECT_EQ(node->coeffs()[0], 1);
  EXPECT_EQ(node->coeffs()[1], 2);
  EXPECT_EQ(node->c(), 3);
}

TEST_F(LinEqNodeTest, application) {
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

  // a, b, the bound (which we have to represent as a variable, but it has a
  // unit domain so a search wouldn't use it).
  EXPECT_EQ(engine.searchVariables().size(), 3);

  // a, b, the linear sum of a and b, the bound (we have to represent it as an
  // IntVar), the violation of the <= constraint.
  EXPECT_EQ(engine.numVariables(), 5);

  // linear and <=
  EXPECT_EQ(engine.numInvariants(), 2);
}
