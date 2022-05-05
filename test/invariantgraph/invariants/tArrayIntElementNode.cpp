#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayIntElementNode.hpp"

class ArrayIntElementNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(b, 0, 10);
  INT_VARIABLE(c, 0, 10);
  std::vector<Int> elementValues{1, 2, 3};

  fznparser::Constraint constraint{
      "array_int_element",
      {"b",
       fznparser::Constraint::ArrayArgument{
           elementValues.at(0), elementValues.at(1), elementValues.at(2)},
       "c"},
      {}};
  fznparser::FZNModel model{{}, {b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayIntElementNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::ArrayIntElementNode>(constraint);
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), {node->b()});

  std::vector<Int> expectedAs{1, 2, 3};
  EXPECT_EQ(node->as(), expectedAs);
}

TEST_F(ArrayIntElementNodeTest, application) {
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

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b and c
  EXPECT_EQ(engine.numVariables(), 2);

  // elementConst
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayIntElementNodeTest, propagation) {
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

    if (0 < value && value <= static_cast<Int>(elementValues.size())) {
      EXPECT_EQ(engine.currentValue(outputId), elementValues.at(value - 1));
    }
  }
}