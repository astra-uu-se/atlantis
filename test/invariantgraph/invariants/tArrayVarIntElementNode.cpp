#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayVarIntElementNode.hpp"

class ArrayVarIntElementNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 3, 10);
  INT_VARIABLE(b, 2, 11);
  INT_VARIABLE(c, 1, 9);

  INT_VARIABLE(idx, 0, 10);
  INT_VARIABLE(y, 0, 10);

  fznparser::Constraint constraint{
      "array_var_int_element",
      {"idx", fznparser::Constraint::ArrayArgument{"a", "b", "c"}, "y"},
      {}};

  fznparser::FZNModel model{
      {}, {a, b, c, idx, y}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayVarIntElementNode> node;

  ArrayVarIntElementNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::ArrayVarIntElementNode>(constraint);
  }
};

TEST_F(ArrayVarIntElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(idx));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(y));
  expectMarkedAsInput(node.get(), {node->as()});
  expectMarkedAsInput(node.get(), {node->b()});

  EXPECT_EQ(node->as().size(), 3);
  EXPECT_EQ(node->as()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(node->as()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->as()[2]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
}

TEST_F(ArrayVarIntElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, idx.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c, idx
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(engine.numVariables(), 5);

  // elementVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayVarIntElementNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, idx.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  EXPECT_EQ(node->inputs().size(), 4);
  for (auto* const inputVariable : node->inputs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->definedVariables()[0]));
  const VarId outputId = _variableMap.at(node->definedVariables()[0]);

  std::vector<VarId> inputs;
  inputs.emplace_back(_variableMap.at(node->b()));
  for (auto* const varNode : node->as()) {
    inputs.emplace_back(_variableMap.at(varNode));
  }

  const VarId input = inputs.front();
  engine.close();
  std::vector<Int> values(4, 0);

  for (values.at(0) = std::max(Int(1), engine.lowerBound(input));
       values.at(0) <= std::min(Int(3), engine.upperBound(input));
       ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      for (values.at(2) = engine.lowerBound(inputs.at(2));
           values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
        for (values.at(3) = engine.lowerBound(inputs.at(3));
             values.at(3) <= engine.upperBound(inputs.at(3)); ++values.at(3)) {
          engine.beginMove();
          for (size_t i = 0; i < inputs.size(); ++i) {
            engine.setValue(inputs.at(i), values.at(i));
          }
          engine.endMove();

          engine.beginProbe();
          engine.query(outputId);
          engine.endProbe();
          const Int actual = engine.currentValue(outputId);
          EXPECT_EQ(actual, values.at(values.at(0)));
        }
      }
    }
  }
}