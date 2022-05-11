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

  void SetUp() override {
    setModel(&model);
    node = invariantgraph::ArrayBoolElementNode::fromModelConstraint(
        *_model, constraint, nodeFactory);
  }
};

TEST_F(ArrayBoolElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
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
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
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
  node->createDefinedVariables(engine);
  node->registerWithEngine(engine);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->staticInputs().size(), 1);
  for (auto* const inputVariable : node->staticInputs()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
    inputs.emplace_back(inputVariable->varId());
  }

  EXPECT_NE(node->definedVariables().front()->varId(), NULL_ID);
  const VarId outputId = node->definedVariables().front()->varId();
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