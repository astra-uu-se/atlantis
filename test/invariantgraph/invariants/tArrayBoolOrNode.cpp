#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/arrayBoolOrNode.hpp"

class ArrayBoolOrNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(r);

  fznparser::Constraint constraint{
      "array_bool_or",
      {fznparser::Constraint::ArrayArgument{a.name, b.name}, r.name},
      {}};
  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::ArrayBoolOrNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::ArrayBoolOrNode>(constraint);
  }
};

TEST_F(ArrayBoolOrNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(r));

  EXPECT_EQ(node->staticInputs().size(), 2);
  EXPECT_EQ(*node->staticInputs()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->staticInputs()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));

  expectMarkedAsInput(node.get(), node->staticInputs());
}

TEST_F(ArrayBoolOrNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 3);

  // minSparse
  EXPECT_EQ(engine.numInvariants(), 1);
}

static bool isViolating(const std::vector<Int>& values) {
  for (const Int val : values) {
    if (val == 0) {
      return false;
    }
  }
  return true;
}

TEST_F(ArrayBoolOrNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->createDefinedVariables(engine);
  node->registerWithEngine(engine);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->staticInputs().size(), 2);
  for (auto* const inputVariable : node->staticInputs()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
    inputs.emplace_back(inputVariable->varId());
  }

  EXPECT_NE(node->definedVariables().front()->varId(), NULL_ID);
  const VarId outputId = node->definedVariables().front()->varId();
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