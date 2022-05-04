#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"

class Bool2IntNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  INT_VARIABLE(b, 0, 1);

  fznparser::Constraint constraint{
      "bool2int", {"a", "b"}, {fznparser::DefinesVariableAnnotation{"b"}}};

  fznparser::FZNModel model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::Bool2IntNode> node;

  Bool2IntNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::Bool2IntNode>(constraint);
  }
};

TEST_F(Bool2IntNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
}

TEST_F(Bool2IntNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);

  // a and b
  EXPECT_EQ(_variableMap.size(), 2);
}

TEST_F(Bool2IntNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(node->inputs().size(), 1);
  EXPECT_TRUE(_variableMap.contains(node->inputs().front()));
  EXPECT_TRUE(_variableMap.contains(node->definedVariables().at(0)));

  const VarId input = _variableMap.at(node->inputs().front());
  const VarId outputId = _variableMap.at(node->definedVariables().at(0));

  for (Int value = engine.lowerBound(input); value <= engine.upperBound(input);
       ++value) {
    engine.beginMove();
    engine.setValue(input, value);
    engine.endMove();

    engine.beginProbe();
    engine.query(outputId);
    engine.endProbe();

    const Int expected = (value == 0);
    const Int actual = engine.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}
