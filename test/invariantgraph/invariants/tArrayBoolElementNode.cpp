#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElementNode.hpp"

class ArrayBoolElementNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::BoolVar> y;
  std::vector<bool> elementValues{true, false, false, true};

  std::unique_ptr<invariantgraph::ArrayIntElementNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    b = intVar(1, 4, "b");
    y = boolVar("y");

    fznparser::BoolVarArray elements("");
    for (const auto& elem : elementValues) {
      elements.append(elem);
    }

    node = makeNode<invariantgraph::ArrayIntElementNode>(
        _model->addConstraint(fznparser::Constraint(
            "array_bool_element",
            std::vector<fznparser::Arg>{fznparser::IntArg{*b}, elements,
                                        fznparser::BoolArg{*y}})));
  }
};

TEST_F(ArrayBoolElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(), invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*y));
  expectMarkedAsInput(node.get(), {node->b()});

  std::vector<Int> expectedAs{0, 1, 1, 0};
  EXPECT_EQ(node->as(), expectedAs);
}

TEST_F(ArrayBoolElementNodeTest, application) {
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

  // The index ranges over the as array (first index is 1).
  EXPECT_EQ(engine.lowerBound(engineVariable(*b)), 1);
  EXPECT_EQ(engine.upperBound(engineVariable(*b)), node->as().size());

  // The output domain should contain all elements in as.
  EXPECT_EQ(engine.lowerBound(engineVariable(*y)), 0);
  EXPECT_EQ(engine.upperBound(engineVariable(*y)), 1);

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b (y is a view)
  EXPECT_EQ(engine.numVariables(), 1);

  // elementConst is a view
  EXPECT_EQ(engine.numInvariants(), 0);
}

TEST_F(ArrayBoolElementNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addVariablesToEngine(engine);
  node->registerOutputVariables(engine);
  node->registerNode(*_invariantGraph, engine);

  std::vector<VarId> inputs;
  EXPECT_EQ(node->staticInputVarNodeIds().size(), 1);
  for (auto* const inputVariable : node->staticInputVarNodeIds()) {
    EXPECT_NE(inputVariable->varId(), NULL_ID);
    inputs.emplace_back(inputVariable->varId());
  }

  EXPECT_NE(node->outputVarNodeIds().front()->varId(), NULL_ID);
  const VarId outputId = node->outputVarNodeIds().front()->varId();
  EXPECT_EQ(inputs.size(), 1);

  const VarId input = inputs.front();
  engine.close();

  for (Int value = engine.lowerBound(input); value <= engine.upperBound(input);
       ++value) {
    engine.beginMove();
    engine.setValue(input, value);
    engine.endMove();

    engine.beginProbe();
    engine.query(outputId);
    engine.endProbe();

    EXPECT_EQ(engine.currentValue(outputId), !elementValues.at(value - 1));
  }
}