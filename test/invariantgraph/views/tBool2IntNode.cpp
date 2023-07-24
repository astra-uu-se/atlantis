#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"

class Bool2IntNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  INT_VARIABLE(b, 0, 1);

  fznparser::Constraint constraint{
      "bool2int", {"a", "b"}, {fznparser::DefinesVariableAnnotation{"b"}}};

  fznparser::Model model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::Bool2IntNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::Bool2IntNode>(constraint);
  }
};

TEST_F(Bool2IntNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
}

TEST_F(Bool2IntNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);
}

TEST_F(Bool2IntNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name});
  node->createDefinedVariables(engine);
  node->registerWithEngine(engine);
  engine.close();

  EXPECT_EQ(node->staticInputs().size(), 1);
  EXPECT_NE(node->staticInputs().front()->varId(), NULL_ID);
  EXPECT_NE(node->definedVariables().front()->varId(), NULL_ID);

  const VarId input = node->staticInputs().front()->varId();
  const VarId outputId = node->definedVariables().front()->varId();

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
