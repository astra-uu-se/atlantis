#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intModNode.hpp"

class IntModNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;

  std::unique_ptr<invariantgraph::IntModNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 6);
    b = FZN_SEARCH_VARIABLE("b", 1, 10);
    c = FZN_SEARCH_VARIABLE("c", 0, 10);

    FZN_DEFINES_VAR_ANNOTATION(annotations, c);
    auto constraint = makeConstraint("int_mod", annotations, a, b, c);

    node = invariantgraph::BinaryOpNode::fromModelConstraint<
        invariantgraph::IntModNode>(constraint, nodeFactory);
  }
};

TEST_F(IntModNodeTest, construction) {
  EXPECT_EQ(node->a()->variable(), a);
  EXPECT_EQ(node->b()->variable(), b);
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), c);
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntModNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a, b});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  EXPECT_EQ(engine.getLowerBound(engineVariable(c)), 0);
  EXPECT_EQ(engine.getUpperBound(engineVariable(c)), 6);

  // a and b
  EXPECT_EQ(engine.getDecisionVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.getNumVariables(), 3);

  // intMod
  EXPECT_EQ(engine.getNumInvariants(), 1);
}
