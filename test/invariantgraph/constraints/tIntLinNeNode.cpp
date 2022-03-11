#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intLinNeNode.hpp"

class IntLinNeNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;

  std::unique_ptr<invariantgraph::IntLinNeNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 10);
    b = FZN_SEARCH_VARIABLE("b", 0, 10);
    c = FZN_VALUE(3);

    auto constraint =
        makeConstraint("int_lin_ne", FZN_NO_ANNOTATIONS,
                       FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(2)),
                       FZN_VECTOR_CONSTRAINT_ARG(a, b), c);

    node = makeNode<invariantgraph::IntLinNeNode>(constraint);
  }
};

TEST_F(IntLinNeNodeTest, construction) {
  EXPECT_EQ(node->variables()[0]->variable(), a);
  EXPECT_EQ(node->variables()[1]->variable(), b);
  EXPECT_EQ(node->coeffs()[0], 1);
  EXPECT_EQ(node->coeffs()[1], 2);
  EXPECT_EQ(node->c(), 3);
  expectMarkedAsInput(node.get(), node->variables());
}

TEST_F(IntLinNeNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, the bound (which we have to represent as a variable, but it has a
  // unit domain so a search wouldn't use it).
  EXPECT_EQ(engine.getDecisionVariables().size(), 3);

  // a, b, the linear sum of a and b, the bound (we have to represent it as an
  // IntVar), the violation of the != constraint.
  EXPECT_EQ(engine.getNumVariables(), 5);

  // linear and !=
  EXPECT_EQ(engine.getNumInvariants(), 2);
}
