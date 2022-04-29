#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/eqReifNode.hpp"

class EqReifNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(r, 0, 1);

  fznparser::Constraint constraint{"int_eq_reif", {"a", "b", "r"}, {}};

  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::EqReifNode> node;

  EqReifNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::EqReifNode>(constraint);
  }
};

TEST_F(EqReifNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(r));
}

TEST_F(EqReifNodeTest, application) {
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

  // a, b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 3);
}

static Int isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) != values.at(j)) {
        return 1;
      }
    }
  }
  return 0;
}

TEST_F(EqReifNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->inputs().size(), 2);
  for (auto* const inputVariable : node->inputs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    inputs.emplace_back(_variableMap.at(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->definedVariables().at(0)));
  const VarId outputId = _variableMap.at(node->definedVariables().at(0));
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

      EXPECT_EQ(engine.currentValue(outputId), isViolating(values));
    }
  }
}