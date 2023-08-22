#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"

class ArrayVarBoolElementNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::BoolVar> a;
  std::unique_ptr<fznparser::BoolVar> b;
  std::unique_ptr<fznparser::BoolVar> c;

  std::unique_ptr<fznparser::IntVar> idx;
  std::unique_ptr<fznparser::IntVar> y;

  std::unique_ptr<invariantgraph::ArrayVarBoolElementNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = boolVar("a");
    b = boolVar("b");
    c = boolVar("c");
    idx = intVar(0, 10, "idx");
    y = intVar(0, 10, "y");

    fznparser::BoolVarArray inputs("");
    inputs.append(*a);
    inputs.append(*b);
    inputs.append(*c);

    node = makeNode<invariantgraph::ArrayVarBoolElementNode>(
        _model->addConstraint(fznparser::Constraint(
            "array_var_bool_element",
            std::vector<fznparser::Arg>{fznparser::IntArg{*idx}, inputs,
                                        fznparser::IntArg{*y}})));
  }
};

TEST_F(ArrayVarBoolElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(), invariantgraph::VarNode::FZNVariable(*idx));
  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*y));
  expectMarkedAsInput(node.get(), {node->dynamicInputVarNodeIds()});
  expectMarkedAsInput(node.get(), {node->b()});

  EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 3);
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(0)->variable(),
            invariantgraph::VarNode::FZNVariable(*a));
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(1)->variable(),
            invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(node->dynamicInputVarNodeIds().at(2)->variable(),
            invariantgraph::VarNode::FZNVariable(*c));
}

TEST_F(ArrayVarBoolElementNodeTest, application) {
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

  // a, b, c, idx
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, idx, y
  EXPECT_EQ(engine.numVariables(), 5);

  // elementVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayVarBoolElementNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  node->registerOutputVariables(engine);
  node->registerNode(*_invariantGraph, engine);

  EXPECT_EQ(node->staticInputVarNodeIds().size(), 1);
  EXPECT_NE(node->staticInputVarNodeIds().front()->varId(), NULL_ID);

  EXPECT_EQ(node->dynamicInputVarNodeIds().size(), 3);
  for (auto* const inputVariable : node->dynamicInputVarNodeIds()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
  }

  EXPECT_NE(node->outputVarNodeIds().front()->varId(), NULL_ID);
  const VarId outputId = node->outputVarNodeIds().front()->varId();

  std::vector<VarId> inputs;
  inputs.emplace_back(node->staticInputVarNodeIds().front()->varId());
  for (auto* const varNode : node->dynamicInputVarNodeIds()) {
    inputs.emplace_back(varNode->varId());
    engine.updateBounds(varNode->varId(), 0, 10, true);
  }
  engine.close();
  std::vector<Int> values(4, 0);

  for (values.at(0) = std::max(Int(1), engine.lowerBound(inputs.at(0)));
       values.at(0) <= std::min(Int(3), engine.upperBound(inputs.at(0)));
       ++values.at(0)) {
    for (values.at(1) = 0; values.at(1) <= 1; ++values.at(1)) {
      for (values.at(2) = 0; values.at(2) <= 1; ++values.at(2)) {
        for (values.at(3) = 0; values.at(3) <= 1; ++values.at(3)) {
          engine.beginMove();
          for (size_t i = 0; i < inputs.size(); ++i) {
            engine.setValue(inputs.at(i), values.at(i));
          }
          engine.endMove();

          engine.beginProbe();
          engine.query(outputId);
          engine.endProbe();
          EXPECT_EQ(engine.currentValue(outputId), values.at(values.at(0)) > 0);
        }
      }
    }
  }
}