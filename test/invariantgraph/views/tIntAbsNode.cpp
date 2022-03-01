#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intAbsNode.hpp"

class IntAbsNodeTest : public NodeTestBase {
 public:
  std::shared_ptr<fznparser::SearchVariable> a;
  std::shared_ptr<fznparser::SearchVariable> b;

  std::unique_ptr<invariantgraph::IntAbsNode> node;

  void SetUp() override {
    a = FZN_SEARCH_VARIABLE("a", 5, 10);
    b = FZN_SEARCH_VARIABLE("b", 2, 7);

    auto constraint = FZN_CONSTRAINT("int_abs", FZN_NO_ANNOTATIONS, a, b);
    node = invariantgraph::IntAbsNode::fromModelConstraint(constraint,
                                                           nodeFactory);
  }
};

TEST_F(IntAbsNodeTest, construction) {
  EXPECT_EQ(node->input(), a);
  EXPECT_EQ(node->variable(), b);
}

TEST_F(IntAbsNodeTest, application) {
  std::map<invariantgraph::VariableNode*, VarId> variableMap;

  PropagationEngine engine;
  engine.open();
  _variables[0]->registerWithEngine(engine, variableMap);
  node->registerWithEngine(engine, variableMap);
  engine.close();

  // a
  EXPECT_EQ(engine.getDecisionVariables().size(), 1);

  // a
  EXPECT_EQ(engine.getNumVariables(), 1);

  // a and b
  EXPECT_EQ(variableMap.size(), 2);
}
