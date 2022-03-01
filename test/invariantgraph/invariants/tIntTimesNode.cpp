#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariants/intTimesNode.hpp"

class IntTimesNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;
  std::shared_ptr<fznparser::SearchVariable> c;

  std::vector<std::unique_ptr<invariantgraph::VariableNode>> _variables;
  std::unique_ptr<invariantgraph::IntTimesNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 0, 10);
    b = FZN_SEARCH_VARIABLE("b", 0, 10);
    c = FZN_SEARCH_VARIABLE("c", 0, 10);

    FZN_DEFINES_VAR_ANNOTATION(annotations, c);
    auto constraint = makeConstraint("int_times", annotations, a, b, c);

    node = invariantgraph::BinaryOpNode::fromModelConstraint<
        invariantgraph::IntTimesNode>(constraint, nodeFactory);
  }
};

TEST_F(IntTimesNodeTest, construction) {
  EXPECT_EQ(node->a()->variable(), a);
  EXPECT_EQ(node->b()->variable(), b);
  EXPECT_EQ(node->output()->variable(), c);
}

TEST_F(IntTimesNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  node->registerWithEngine(engine, [&](auto var) {
    auto domain = var->variable()->domain();
    return engine.makeIntVar(5, domain->lowerBound(), domain->upperBound());
  });
  engine.close();

  // a and b
  EXPECT_EQ(engine.getDecisionVariables().size(), 2);

  // a, b and c
  EXPECT_EQ(engine.getNumVariables(), 3);

  // intTimes
  EXPECT_EQ(engine.getNumInvariants(), 1);
}
