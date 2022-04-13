#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/allDifferentNode.hpp"

class AllDifferentNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(c, 2, 7);
  INT_VARIABLE(d, 2, 7);

  fznparser::Constraint constraint{
      "alldifferent",
      {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
      {}};
  fznparser::FZNModel model{
      {}, {a, b, c, d}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::AllDifferentNode> node;

  AllDifferentNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::AllDifferentNode>(constraint);
  }
};

TEST_F(AllDifferentNodeTest, construction) {
  std::vector<invariantgraph::VariableNode*> expectedVars;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(expectedVars),
                 [](const auto& variable) { return variable.get(); });

  EXPECT_EQ(node->variables(), expectedVars);
  expectMarkedAsInput(node.get(), node->variables());
}

TEST_F(AllDifferentNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, d.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, d and the violation
  EXPECT_EQ(engine.numVariables(), 5);

  // alldifferent
  EXPECT_EQ(engine.numInvariants(), 1);

  EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
  EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 3);
}
