#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intAbsNode.hpp"

class IntAbsNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> a;
  std::unique_ptr<fznparser::IntVar> b;

  std::unique_ptr<invariantgraph::IntAbsNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = intVar(5, 10, "a");
    b = intVar(2, 7, "b");

    node = makeNode<invariantgraph::IntAbsNode>(_model->addConstraint(
        fznparser::Constraint("int_abs",
                              std::vector<fznparser::Arg>{
                                  fznparser::IntArg{*a}, fznparser::IntArg{*b}},
                              std::vector<fznparser::Annotation>{
                                  definesVarAnnotation(b->identifier())})));
  }
};

TEST_F(IntAbsNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(),
            invariantgraph::VarNode::FZNVariable(*a));
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*b));
}

TEST_F(IntAbsNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_EQ(definedVariable->varId(), NULL_ID);
  }
  node->registerOutputVariables(engine);
  for (auto* const definedVariable : node->outputVarNodeIds()) {
    EXPECT_NE(definedVariable->varId(), NULL_ID);
  }
  node->registerNode(*_invariantGraph, engine);
  engine.close();

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);
}