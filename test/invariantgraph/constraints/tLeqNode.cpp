#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/leqNode.hpp"

class LeqNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;

  std::unique_ptr<invariantgraph::LeqNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 10);
    b = FZN_SEARCH_VARIABLE("b", 0, 10);
    c = FZN_VALUE(3);

    auto constraint = FZN_CONSTRAINT(
        "int_lin_le", FZN_NO_ANNOTATIONS,
        FZN_VECTOR_CONSTRAINT_ARG(FZN_CONSTRAINT_ARG(FZN_VALUE(1)),
                                  FZN_CONSTRAINT_ARG(FZN_VALUE(2))),
        FZN_VECTOR_CONSTRAINT_ARG(FZN_CONSTRAINT_ARG(a), FZN_CONSTRAINT_ARG(b)),
        FZN_CONSTRAINT_ARG(c));

    node =
        invariantgraph::LeqNode::fromModelConstraint(constraint, nodeFactory);
  }
};

TEST_F(LeqNodeTest, construction) {
  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->coeffs()[0], 1);
  EXPECT_EQ(node->coeffs()[1], 2);
  EXPECT_EQ(node->bound(), 3);
}

TEST_F(LeqNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(0, domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // a, b, the bound (which we have to represent as a variable, but it has a
  // unit domain so a search wouldn't use it).
  EXPECT_EQ(engine.getDecisionVariables().size(), 3);

  // a, b, the linear sum of a and b, the bound (we have to represent it as an
  // IntVar), the violation of the <= constraint.
  EXPECT_EQ(engine.getNumVariables(), 5);

  // linear and <=
  EXPECT_EQ(engine.getNumInvariants(), 2);
}
