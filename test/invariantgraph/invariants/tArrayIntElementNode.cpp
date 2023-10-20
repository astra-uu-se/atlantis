#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"

class ArrayIntElementNodeTest
    : public NodeTestBase<invariantgraph::ArrayIntElementNode> {
 public:
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId c;
  std::vector<Int> elementValues{1, 2, 3};

  void SetUp() override {
    NodeTestBase::SetUp();
    b = createIntVar(0, 10, "b");
    c = createIntVar(0, 10, "c");

    fznparser::IntVarArray elements("");
    for (const auto& elem : elementValues) {
      elements.append(elem);
    }

    _model->addConstraint(fznparser::Constraint(
        "array_int_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(b)}, elements,
                                    fznparser::IntArg{intVar(c)}}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayIntElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().b(), b);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), c);

  std::vector<Int> expectedAs{1, 2, 3};
  EXPECT_EQ(invNode().as(), expectedAs);
}

TEST_F(ArrayIntElementNodeTest, application) {
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

    if (0 < value && value <= static_cast<Int>(elementValues.size())) {
      EXPECT_EQ(engine.currentValue(outputId), elementValues.at(value - 1));
    }
  }
}