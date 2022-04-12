#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intTimesNode.hpp"

class IntTimesNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 10);
  INT_VARIABLE(b, 0, 10);
  INT_VARIABLE(c, 0, 10);

  fznparser::Constraint constraint{"int_times",
                                   {"a", "b", "c"},
                                   {fznparser::DefinesVariableAnnotation{"c"}}};

  fznparser::FZNModel model{{}, {a, b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntTimesNode> node;

  IntTimesNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = invariantgraph::BinaryOpNode::fromModelConstraint<
        invariantgraph::IntTimesNode>(_model, constraint, nodeFactory);
  }
};

TEST_F(IntTimesNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntTimesNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(c)), 0);
  EXPECT_EQ(engine.upperBound(engineVariable(c)), 100);

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // intTimes
  EXPECT_EQ(engine.numInvariants(), 1);
}
