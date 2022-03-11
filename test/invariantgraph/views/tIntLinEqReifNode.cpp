#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intLinEqReifNode.hpp"

class IntLinEqReifNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;
  std::shared_ptr<fznparser::SearchVariable> r;

  std::unique_ptr<invariantgraph::IntLinEqReifNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    c = FZN_VALUE(3);
    r = FZN_SEARCH_VARIABLE("r", 0, 1);

    auto as = FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(-1));
    auto bs = FZN_VECTOR_CONSTRAINT_ARG(a, b);

    auto constraint =
        makeConstraint("int_lin_eq_reif", FZN_NO_ANNOTATIONS, as, bs, c, r);

    node = makeNode<invariantgraph::IntLinEqReifNode>(constraint);
  }
};

TEST_F(IntLinEqReifNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), r);
}

TEST_F(IntLinEqReifNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.getDecisionVariables().size(), 3);

  // a, b and r
  EXPECT_EQ(engine.getNumVariables(), 5);
}
