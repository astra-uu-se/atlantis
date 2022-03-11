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
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntNeNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a and b
  EXPECT_EQ(engine.getDecisionVariables().size(), 2);

  // a, b and the violation
  EXPECT_EQ(engine.getNumVariables(), 3);

  // notEqual
  EXPECT_EQ(engine.getNumInvariants(), 1);

  EXPECT_EQ(engine.getLowerBound(_variableMap.at(node->violation())), 0);
  EXPECT_EQ(engine.getUpperBound(_variableMap.at(node->violation())), 1);
}
