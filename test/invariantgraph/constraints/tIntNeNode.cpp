#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intNeNode.hpp"

class IntNeNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;

  std::unique_ptr<invariantgraph::IntNeNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);

    auto constraint = makeConstraint("int_ne", FZN_NO_ANNOTATIONS, a, b);

    node = makeNode<invariantgraph::IntNeNode>(constraint);
  }
};

TEST_F(IntNeNodeTest, construction) {
  EXPECT_EQ(node->a()->variable(), a);
  EXPECT_EQ(node->b()->variable(), b);

  EXPECT_NE(std::find(node->a()->softConstraints().begin(),
                      node->a()->softConstraints().end(), node.get()),
            node->a()->softConstraints().end());
  EXPECT_NE(std::find(node->b()->softConstraints().begin(),
                      node->b()->softConstraints().end(), node.get()),
            node->b()->softConstraints().end());
}

TEST_F(IntNeNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  VarId violation = node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(0, domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // a and b
  EXPECT_EQ(engine.getDecisionVariables().size(), 2);

  // a, b and the violation
  EXPECT_EQ(engine.getNumVariables(), 3);

  // notEqual
  EXPECT_EQ(engine.getNumInvariants(), 1);

  EXPECT_EQ(engine.getLowerBound(violation), 0);
  EXPECT_EQ(engine.getUpperBound(violation), 1);
}
