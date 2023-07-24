#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

class CircuitImplicitNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 1, 4);
  INT_VARIABLE(b, 1, 4);
  INT_VARIABLE(c, 1, 4);
  INT_VARIABLE(d, 1, 4);

  fznparser::Constraint constraint{
      "circuit_no_offset",
      {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
      {}};

  fznparser::Model model{{}, {a, b, c, d}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::CircuitImplicitNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::CircuitImplicitNode>(constraint);
  }
};

TEST_F(CircuitImplicitNodeTest, construction) {
  std::vector<invariantgraph::VariableNode*> expectedVars;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(expectedVars),
                 [](const auto& variable) { return variable.get(); });

  EXPECT_EQ(node->definedVariables(), expectedVars);
}

TEST_F(CircuitImplicitNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c and d
  EXPECT_EQ(engine.numVariables(), 4);

  EXPECT_EQ(engine.numInvariants(), 0);

  auto neighbourhood = node->takeNeighbourhood();
  EXPECT_FALSE(node->takeNeighbourhood());

  EXPECT_TRUE(dynamic_cast<search::neighbourhoods::CircuitNeighbourhood*>(
      neighbourhood.get()));
}
