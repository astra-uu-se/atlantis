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

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::IntAbsNode>(constraint);
  }
};

TEST_F(IntAbsNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables().front()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
}

TEST_F(IntAbsNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->createDefinedVariables(engine);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerWithEngine(engine);
  engine.close();

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);
}