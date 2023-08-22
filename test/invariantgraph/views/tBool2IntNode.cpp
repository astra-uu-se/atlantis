#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"

class Bool2IntNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::BoolVar> a;
  std::unique_ptr<fznparser::IntVar> b;

  std::unique_ptr<invariantgraph::Bool2IntNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = boolVar("a");
    b = intVar(0, 1, "b");

    node = makeNode<invariantgraph::Bool2IntNode>(
        _model->addConstraint(fznparser::Constraint(
            "bool2int",
            std::vector<fznparser::Arg>{fznparser::BoolArg{*a},
                                        fznparser::IntArg{*b}},
            std::vector<fznparser::Annotation>{
                definesVarAnnotation(b->identifier())})));
  }
};

TEST_F(Bool2IntNodeTest, construction) {
  EXPECT_EQ(node->input()->variable(),
            invariantgraph::VarNode::FZNVariable(*a));
  EXPECT_EQ(node->input()->inputFor().size(), 1);
  EXPECT_EQ(node->input()->inputFor()[0], node.get());

  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*b));
}

TEST_F(Bool2IntNodeTest, application) {
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

TEST_F(Bool2IntNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  node->registerOutputVariables(engine);
  node->registerNode(*_invariantGraph, engine);
  engine.close();

  EXPECT_EQ(node->staticInputVarNodeIds().size(), 1);
  EXPECT_NE(node->staticInputVarNodeIds().front()->varId(), NULL_ID);
  EXPECT_NE(node->outputVarNodeIds().front()->varId(), NULL_ID);

  const VarId input = node->staticInputVarNodeIds().front()->varId();
  const VarId outputId = node->outputVarNodeIds().front()->varId();

  for (Int value = engine.lowerBound(input); value <= engine.upperBound(input);
       ++value) {
    engine.beginMove();
    engine.setValue(input, value);
    engine.endMove();

    engine.beginProbe();
    engine.query(outputId);
    engine.endProbe();

    const Int expected = (value == 0);
    const Int actual = engine.currentValue(outputId);
    EXPECT_EQ(expected, actual);
  }
}
