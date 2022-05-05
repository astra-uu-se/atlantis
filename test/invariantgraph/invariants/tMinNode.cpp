#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/minNode.hpp"

class MinNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, -5, 10);
  INT_VARIABLE(b, 0, 5);
  INT_VARIABLE(c, 0, 10);

  fznparser::Constraint constraint{
      "array_int_minimum",
      {"c", fznparser::Constraint::ArrayArgument{"a", "b"}},
      {fznparser::DefinesVariableAnnotation{"c"}}};

  fznparser::FZNModel model{{}, {a, b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::MinNode> node;

  MinNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::MinNode>(constraint);
  }
};

TEST_F(MinNodeTest, construction) {
  EXPECT_EQ(*node->staticInputs()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->staticInputs()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), node->staticInputs());
}

TEST_F(MinNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(engine.lowerBound(engineVariable(c)), -5);
  EXPECT_EQ(engine.upperBound(engineVariable(c)), 5);

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // minSparse
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(MinNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->staticInputs().size(), 2);
  for (auto* const inputVariable : node->staticInputs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    inputs.emplace_back(_variableMap.at(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->definedVariables().at(0)));
  const VarId outputId = _variableMap.at(node->definedVariables().at(0));
  EXPECT_EQ(inputs.size(), 2);

  std::vector<Int> values(inputs.size());
  engine.close();

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      engine.beginMove();
      for (size_t i = 0; i < inputs.size(); ++i) {
        engine.setValue(inputs.at(i), values.at(i));
      }
      engine.endMove();

      engine.beginProbe();
      engine.query(outputId);
      engine.endProbe();

      const Int expected = *std::min_element(values.begin(), values.end());
      const Int actual = engine.currentValue(outputId);
      EXPECT_EQ(expected, actual);
    }
  }
}