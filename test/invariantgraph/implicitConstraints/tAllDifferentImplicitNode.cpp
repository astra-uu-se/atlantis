#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "search/neighbourhoods/allDifferentNeighbourhood.hpp"

class AllDifferentImplicitNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;
  std::shared_ptr<fznparser::SearchVariable> d;

  std::unique_ptr<invariantgraph::AllDifferentImplicitNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 2, 7);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    c = FZN_SEARCH_VARIABLE("c", 2, 7);
    d = FZN_SEARCH_VARIABLE("d", 2, 7);

    auto constraint = makeConstraint("alldifferent", FZN_NO_ANNOTATIONS,
                                     FZN_VECTOR_CONSTRAINT_ARG(a, b, c, d));

    node = makeNode<invariantgraph::AllDifferentImplicitNode>(constraint);
  }
};

TEST_F(AllDifferentImplicitNodeTest, construction) {
  std::vector<invariantgraph::VariableNode*> expectedVars;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(expectedVars),
                 [](const auto& variable) { return variable.get(); });

  EXPECT_EQ(node->definedVariables(), expectedVars);
  EXPECT_FALSE(node->violation());
}

TEST_F(AllDifferentImplicitNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c and d
  EXPECT_EQ(engine.numVariables(), 4);

  EXPECT_EQ(engine.numInvariants(), 0);

  auto neighbourhood = node->takeNeighbourhood();
  EXPECT_FALSE(node->takeNeighbourhood());

  EXPECT_TRUE(dynamic_cast<search::neighbourhoods::AllDifferentNeighbourhood*>(
      neighbourhood.get()));
}
