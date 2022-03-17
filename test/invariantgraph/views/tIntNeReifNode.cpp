#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intNeReifNode.hpp"

class IntNeReifNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> r;

  std::unique_ptr<invariantgraph::IntNeReifNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);
    r = FZN_SEARCH_VARIABLE("r", 0, 1);

    auto constraint = makeConstraint("int_ne_reif", FZN_NO_ANNOTATIONS, a, b, r);

    node = makeNode<invariantgraph::IntNeReifNode>(constraint);
  }
};

TEST_F(IntNeReifNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), r);
}

TEST_F(IntNeReifNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 3);
}
