#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElementNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"

class ArrayBoolElementNodeTest
    : public NodeTestBase<invariantgraph::ArrayIntElementNode> {
 public:
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId y;
  std::vector<bool> elementValues{true, false, false, true};

  void SetUp() override {
    NodeTestBase::SetUp();
    b = createIntVar(1, 4, "b");
    y = createBoolVar("y");

    fznparser::BoolVarArray elements("");
    for (const auto& elem : elementValues) {
      elements.append(elem);
    }

    _model->addConstraint(fznparser::Constraint(
        "array_bool_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(b)}, elements,
                                    fznparser::BoolArg{boolVar(y)}}));

    makeOtherInvNode<invariantgraph::ArrayBoolElementNode>(
        _model->constraints().front());
  }
};

TEST_F(ArrayBoolElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().b(), b);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), y);

  std::vector<Int> expectedAs{0, 1, 1, 0};
  EXPECT_EQ(invNode().as(), expectedAs);
}

TEST_F(ArrayBoolElementNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerOutputVariables(*_invariantGraph, engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, engine);
  engine.close();

  // The index ranges over the as array (first index is 1).
  EXPECT_EQ(engine.lowerBound(varId(b)), 1);
  EXPECT_EQ(engine.upperBound(varId(b)), invNode().as().size());

  // The output domain should contain all elements in as.
  EXPECT_EQ(engine.lowerBound(varId(y)), 0);
  EXPECT_EQ(engine.upperBound(varId(y)), 1);

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
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);

  std::vector<VarId> inputs;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), NULL_ID);
  const VarId outputId = varId(invNode().outputVarNodeIds().front());
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