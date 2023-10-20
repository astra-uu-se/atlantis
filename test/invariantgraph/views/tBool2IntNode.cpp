#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/bool2IntNode.hpp"

class Bool2IntNodeTest : public NodeTestBase<invariantgraph::Bool2IntNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createIntVar(0, 1, "b");

    _model->addConstraint(fznparser::Constraint(
        "bool2int",
        std::vector<fznparser::Arg>{fznparser::BoolArg{boolVar(a)},
                                    fznparser::IntArg{intVar(b)}},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(b))}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(Bool2IntNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), a);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), b);
}

TEST_F(Bool2IntNodeTest, application) {
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

  // a
  EXPECT_EQ(engine.searchVariables().size(), 1);

  // a
  EXPECT_EQ(engine.numVariables(), 1);
}

TEST_F(Bool2IntNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);
  engine.close();

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), NULL_ID);
  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), NULL_ID);

  const VarId input = varId(invNode().staticInputVarNodeIds().front());
  const VarId outputId = varId(invNode().outputVarNodeIds().front());

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
