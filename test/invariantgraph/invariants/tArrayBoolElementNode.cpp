#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayBoolElementNode.hpp"

class ArrayBoolElementNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(b, 1, 4);
  BOOL_VARIABLE(r);

  fznparser::Constraint constraint{
      "array_bool_element",
      {b.name, fznparser::Constraint::ArrayArgument{true, false, false, true},
       r.name},
      {}};
  fznparser::FZNModel model{{}, {b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayIntElementNode> node;

  ArrayBoolElementNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = invariantgraph::ArrayBoolElementNode::fromModelConstraint(
        model, constraint, nodeFactory);
  }
};

TEST_F(ArrayBoolElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(r));
  expectMarkedAsInput(node.get(), {node->b()});

  std::vector<Int> expectedAs{0, 1, 1, 0};
  EXPECT_EQ(node->as(), expectedAs);
}

TEST_F(ArrayBoolElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // The index ranges over the as array (first index is 1).
  EXPECT_EQ(engine.lowerBound(engineVariable(b)), 1);
  EXPECT_EQ(engine.upperBound(engineVariable(b)), node->as().size());

  // The output domain should contain all elements in as.
  EXPECT_EQ(engine.lowerBound(engineVariable(r)), 0);
  EXPECT_EQ(engine.upperBound(engineVariable(r)), 1);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b and r
  EXPECT_EQ(engine.numVariables(), 2);

  // elementConst
  EXPECT_EQ(engine.numInvariants(), 1);
}