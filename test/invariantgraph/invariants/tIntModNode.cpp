#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intModNode.hpp"

class IntModNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 0, 6);
  INT_VARIABLE(b, 1, 10);
  INT_VARIABLE(c, 0, 10);

  fznparser::Constraint constraint{
      "int_mod", {"a", "b", "c"}, {fznparser::DefinesVariableAnnotation{"c"}}};

  fznparser::Model model{{}, {a, b, c}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntModNode> node;

  void SetUp() override {
    setModel(&model);
    node = invariantgraph::IntModNode::fromModelConstraint(*_model, constraint,
                                                           nodeFactory);
  }
};

TEST_F(IntModNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntModNodeTest, application) {
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

  // a, b and c
  EXPECT_EQ(engine.numVariables(), 3);

  // intMod
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(IntModNodeTest, propagation) {
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

      if (values.at(1) != 0) {
        const Int remainder = engine.currentValue(outputId);
        EXPECT_EQ(remainder, values.at(0) % values.at(1));
      }
    }
  }
}