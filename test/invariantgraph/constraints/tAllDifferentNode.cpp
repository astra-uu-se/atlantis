#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/allDifferentNode.hpp"

class AllDifferentNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;
  std::shared_ptr<fznparser::SearchVariable> d;

  std::unique_ptr<invariantgraph::AllDifferentNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    c = FZN_SEARCH_VARIABLE("c", 2, 7);
    d = FZN_SEARCH_VARIABLE("d", 2, 7);

    auto constraint = makeConstraint("alldifferent", FZN_NO_ANNOTATIONS,
                                     FZN_VECTOR_CONSTRAINT_ARG(a, b, c, d));

    node = makeNode<invariantgraph::AllDifferentNode>(constraint);
  }
};

TEST_F(AllDifferentNodeTest, construction) {
  std::vector<invariantgraph::VariableNode*> expectedVars;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(expectedVars),
                 [](const auto& variable) { return variable.get(); });

  EXPECT_EQ(node->variables(), expectedVars);
}

TEST_F(AllDifferentNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  VarId violation = node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(domain->lowerBound(), domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.getDecisionVariables().size(), 4);

  // a, b, c, d and the violation
  EXPECT_EQ(engine.getNumVariables(), 5);

  // alldifferent
  EXPECT_EQ(engine.getNumInvariants(), 1);

  EXPECT_EQ(engine.getLowerBound(violation), 0);
  EXPECT_EQ(engine.getUpperBound(violation), 4);
}
