#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intLinNeReifNode.hpp"

class IntLinNeReifNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::ValueLiteral> c;
  std::shared_ptr<fznparser::SearchVariable> r;

  std::unique_ptr<invariantgraph::IntLinNeReifNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    c = FZN_VALUE(3);
    r = FZN_SEARCH_VARIABLE("r", 0, 1);

    auto as = FZN_VECTOR_CONSTRAINT_ARG(FZN_VALUE(1), FZN_VALUE(-1));
    auto bs = FZN_VECTOR_CONSTRAINT_ARG(a, b);

    auto constraint =
        makeConstraint("int_lin_ne_reif", FZN_NO_ANNOTATIONS, as, bs, c, r);

    node = makeNode<invariantgraph::IntLinNeReifNode>(constraint);
  }
};

TEST_F(IntLinNeReifNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), r);
}

TEST_F(IntLinNeReifNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.searchVariables().size(), 3);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 5);
}
