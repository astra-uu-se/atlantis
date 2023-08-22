#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"

class ArrayIntElementNodeTest : public NodeTestBase {
 public:
  std::unique_ptr<fznparser::IntVar> b;
  std::unique_ptr<fznparser::IntVar> c;
  std::vector<Int> elementValues{1, 2, 3};

  std::unique_ptr<invariantgraph::ArrayIntElementNode> node;

  void SetUp() override {
    NodeTestBase::SetUp();
    b = intVar(0, 10, "b");
    c = intVar(0, 10, "c");

    fznparser::IntVarArray elements("");
    for (const auto& elem : elementValues) {
      elements.append(elem);
    }

    node = makeNode<invariantgraph::ArrayIntElementNode>(
        _model->addConstraint(fznparser::Constraint(
            "array_int_element",
            std::vector<fznparser::Arg>{fznparser::IntArg{*b}, elements,
                                        fznparser::IntArg{*c}})));
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  EXPECT_EQ(*node->b()->variable(), invariantgraph::VarNode::FZNVariable(*b));
  EXPECT_EQ(node->outputVarNodeIds().size(), 1);
  EXPECT_EQ(*node->outputVarNodeIds().front()->variable(),
            invariantgraph::VarNode::FZNVariable(*c));
  expectMarkedAsInput(node.get(), {node->b()});

  std::vector<Int> expectedAs{1, 2, 3};
  EXPECT_EQ(node->as(), expectedAs);
}

TEST_F(ArrayIntElementNodeTest, application) {
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

  // b
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // b (c is a view)
  EXPECT_EQ(engine.numVariables(), 1);

  // elementConst is a view
  EXPECT_EQ(engine.numInvariants(), 0);
}

TEST_F(ArrayIntElementNodeTest, propagation) {
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

    if (0 < value && value <= static_cast<Int>(elementValues.size())) {
      EXPECT_EQ(engine.currentValue(outputId), elementValues.at(value - 1));
    }
  }
}