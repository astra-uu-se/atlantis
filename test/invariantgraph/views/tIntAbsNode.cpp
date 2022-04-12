#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intAbsNode.hpp"

class IntAbsNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);

  fznparser::Constraint constraint{
      "int_abs", {"a", "b"}, {fznparser::DefinesVariableAnnotation{"b"}}};

  fznparser::FZNModel model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntAbsNode> node;

  IntAbsNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::IntAbsNode>(constraint);
  }
};

TEST_F(IntAbsNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
}

TEST_F(IntAbsNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);

  // a and b
  EXPECT_EQ(_variableMap.size(), 2);
}
