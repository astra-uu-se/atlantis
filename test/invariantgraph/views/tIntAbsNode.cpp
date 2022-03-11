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

    FZN_DEFINES_VAR_ANNOTATION(annotations, b);
    auto constraint = makeConstraint("int_abs", annotations, a, b);

    node = makeNode<invariantgraph::IntAbsNode>(constraint);
  }
};

TEST_F(IntAbsNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(), a);
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(node->definedVariables()[0]->variable(), b);
}

TEST_F(IntAbsNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a
  EXPECT_EQ(engine.getDecisionVariables().size(), 1);

  // a
  EXPECT_EQ(engine.getNumVariables(), 1);

  // a and b
  EXPECT_EQ(_variableMap.size(), 2);
}
