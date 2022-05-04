#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/boolClauseNode.hpp"

class BoolClauseNodeTest : public NodeTestBase {
 public:
  BOOL_VARIABLE(a);
  BOOL_VARIABLE(b);
  BOOL_VARIABLE(c);
  BOOL_VARIABLE(d);

  fznparser::Constraint constraint{
      "bool_clause",
      {fznparser::Constraint::ArrayArgument{a.name, b.name},
       fznparser::Constraint::ArrayArgument{c.name, d.name}},
      {}};
  fznparser::FZNModel model{
      {}, {a, b, c, d}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::BoolClauseNode> node;

  BoolClauseNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::BoolClauseNode>(constraint);
  }
};

TEST_F(BoolClauseNodeTest, construction) {
  EXPECT_EQ(node->as().size(), 2);
  EXPECT_EQ(*node->as()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->as()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));

  EXPECT_EQ(node->bs().size(), 2);
  EXPECT_EQ(*node->bs()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(c));
  EXPECT_EQ(*node->bs()[1]->variable(),
            invariantgraph::VariableNode::FZNVariable(d));

  expectMarkedAsInput(node.get(), node->as());
  expectMarkedAsInput(node.get(), node->bs());
}

TEST_F(BoolClauseNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, d.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  EXPECT_FALSE(_variableMap.contains(node->violation()));
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  EXPECT_TRUE(_variableMap.contains(node->violation()));
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, d, sum
  EXPECT_EQ(engine.numVariables(), 5);

  // linear
  EXPECT_EQ(engine.numInvariants(), 1);
}

static Int computeViolation(const std::vector<Int>& asValues,
                            const std::vector<Int>& bsValues) {
  Int violation = 0;
  for (const Int aVal : asValues) {
    if (aVal != 1) {
      ++violation;
    }
  }
  for (const Int bVal : bsValues) {
    if (bVal != 0) {
      ++violation;
    }
  }
  return violation;
}

TEST_F(BoolClauseNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, d.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  std::vector<VarId> asInputs;
  for (auto* const inputVariable : node->as()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    asInputs.emplace_back(_variableMap.at(inputVariable));
  }
  std::vector<VarId> bsInputs;
  for (auto* const inputVariable : node->bs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    bsInputs.emplace_back(_variableMap.at(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->violation()));
  const VarId violationId = _variableMap.at(node->violation());
  EXPECT_EQ(asInputs.size(), 2);
  EXPECT_EQ(bsInputs.size(), 2);

  std::vector<Int> asValues(asInputs.size());
  std::vector<Int> bsValues(bsInputs.size());
  engine.close();

  for (asValues.at(0) = engine.lowerBound(asInputs.at(0));
       asValues.at(0) <= engine.upperBound(asInputs.at(0)); ++asValues.at(0)) {
    for (asValues.at(1) = engine.lowerBound(asInputs.at(1));
         asValues.at(1) <= engine.upperBound(asInputs.at(1));
         ++asValues.at(1)) {
      for (bsValues.at(0) = engine.lowerBound(bsInputs.at(0));
           bsValues.at(0) <= engine.upperBound(bsInputs.at(0));
           ++bsValues.at(0)) {
        for (bsValues.at(1) = engine.lowerBound(bsInputs.at(1));
             bsValues.at(1) <= engine.upperBound(bsInputs.at(1));
             ++bsValues.at(1)) {
          engine.beginMove();
          for (size_t i = 0; i < asInputs.size(); ++i) {
            engine.setValue(asInputs.at(i), asValues.at(i));
          }
          for (size_t i = 0; i < bsInputs.size(); ++i) {
            engine.setValue(bsInputs.at(i), bsValues.at(i));
          }
          engine.endMove();

          engine.beginProbe();
          engine.query(violationId);
          engine.endProbe();

          EXPECT_EQ(engine.currentValue(violationId),
                    computeViolation(asValues, bsValues));
        }
      }
    }
  }
}