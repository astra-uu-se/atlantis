#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/arrayBoolAndNode.hpp"

class ArrayBoolAndNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  fznparser::Constraint constraint{
      "array_bool_and",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, r.name},
      {}};
  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayBoolAndNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::ArrayBoolAndNode>(constraint);
  }
};

TEST_F(ArrayBoolAndNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().at(0)->variable(),
            invariantgraph::VariableNode::FZNVariable(r));

  EXPECT_EQ(node->staticInputs().size(), 2);
  EXPECT_EQ(*node->staticInputs().at(0)->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->staticInputs().at(1)->variable(),
            invariantgraph::VariableNode::FZNVariable(b));

  expectMarkedAsInput(node.get(), node->staticInputs());
}

TEST_F(ArrayBoolAndNodeTest, application) {
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

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 3);

  // sum
  EXPECT_EQ(engine.numInvariants(), 1);
}

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val > 0) {
      return true;
    }
  }
  return false;
}

TEST_F(ArrayBoolAndNodeTest, propagation) {
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

  EXPECT_TRUE(_variableMap.contains(node->definedVariables()[0]));
  const VarId outputId = _variableMap.at(node->definedVariables()[0]);
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

      const Int viol = engine.currentValue(outputId);

      EXPECT_EQ(viol > 0, isViolating(values));
    }
  }
}

TEST_F(ArrayBoolAndNodeTest, r_is_a_constant) {
  _nodeMap.clear();
  _variableMap.clear();
  _variables.clear();

  fznparser::Constraint constraint2{
      "array_bool_and",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, true},
      {}};

  node = makeNode<invariantgraph::ArrayBoolAndNode>(constraint2);
  EXPECT_TRUE(node->violation());

  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  engine.beginMove();
  engine.setValue(engineVariable(a), 0);
  engine.setValue(engineVariable(b), 0);
  engine.endMove();

  engine.beginCommit();
  engine.query(engineVariable(node->violation()));
  engine.endCommit();
  EXPECT_EQ(engine.committedValue(engineVariable(node->violation())), 0);

  engine.beginMove();
  engine.setValue(engineVariable(a), 1);
  engine.setValue(engineVariable(b), 0);
  engine.endMove();

  engine.beginCommit();
  engine.query(engineVariable(node->violation()));
  engine.endCommit();
  EXPECT_GT(engine.committedValue(engineVariable(node->violation())), 0);

  engine.beginMove();
  engine.setValue(engineVariable(a), 1);
  engine.setValue(engineVariable(b), 1);
  engine.endMove();

  engine.beginCommit();
  engine.query(engineVariable(node->violation()));
  engine.endCommit();
  EXPECT_GT(engine.committedValue(engineVariable(node->violation())), 0);
}
