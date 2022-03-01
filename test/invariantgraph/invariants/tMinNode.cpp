#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/minNode.hpp"

class MinNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;

  std::unique_ptr<invariantgraph::MinNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 10);
    b = FZN_SEARCH_VARIABLE("b", 0, 10);
    c = FZN_SEARCH_VARIABLE("c", 0, 10);

    FZN_DEFINES_VAR_ANNOTATION(annotations, c);
    auto constraint = makeConstraint("array_int_minimum", annotations, c,
                                     FZN_VECTOR_CONSTRAINT_ARG(a, b));

    node = makeNode<invariantgraph::MinNode>(constraint);
  }
};

TEST_F(MinNodeTest, construction) {
  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->output()->variable(), c);
}

TEST_F(MinNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(0, domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // a and b
  EXPECT_EQ(engine.getDecisionVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.getNumVariables(), 3);

  // minSparse
  EXPECT_EQ(engine.getNumInvariants(), 1);
}
