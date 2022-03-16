#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intEqNode.hpp"

class IntEqNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;

  std::unique_ptr<invariantgraph::IntEqNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);

    auto constraint = makeConstraint("int_eq", FZN_NO_ANNOTATIONS, a, b);

    node = makeNode<invariantgraph::IntEqNode>(constraint);
  }
};

TEST_F(IntEqNodeTest, construction) {
  EXPECT_EQ(node->a()->variable(), a);
  EXPECT_EQ(node->b()->variable(), b);
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntEqNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and the violation
  EXPECT_EQ(engine.numVariables(), 3);

  // equal
  EXPECT_EQ(engine.numInvariants(), 1);

  EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
  EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 8);
}
