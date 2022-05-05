#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayBoolElementNode.hpp"

class ArrayBoolElementNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(b, 1, 4);
  BOOL_VARIABLE(r);
  std::vector<bool> elementValues{true, false, false, true};

  fznparser::Constraint constraint{
      "array_bool_element",
      {b.name,
       fznparser::Constraint::ArrayArgument{
           elementValues.at(0), elementValues.at(1), elementValues.at(2),
           elementValues.at(3)},
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

TEST_F(ArrayBoolElementNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {b.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->staticInputs().size(), 1);
  for (auto* const inputVariable : node->staticInputs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    inputs.emplace_back(_variableMap.at(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->definedVariables()[0]));
  const VarId outputId = _variableMap.at(node->definedVariables()[0]);
  EXPECT_EQ(inputs.size(), 1);

  const VarId input = inputs.front();
  engine.close();

  for (Int value = engine.lowerBound(input); value <= engine.upperBound(input);
       ++value) {
    engine.beginMove();
    engine.setValue(input, value);
    engine.endMove();

    engine.beginProbe();
    engine.query(outputId);
    engine.endProbe();

    EXPECT_EQ(engine.currentValue(outputId), !elementValues.at(value - 1));
  }
}