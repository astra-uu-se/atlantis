#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intAbsNode.hpp"

class IntAbsNodeTest : public NodeTestBase<invariantgraph::IntAbsNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(5, 10, "a");
    b = createIntVar(2, 7, "b");

    _model->addConstraint(fznparser::Constraint(
        "int_abs",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(a)},
                                    fznparser::IntArg{intVar(b)}},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(b))}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(IntAbsNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().input(), a);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), b);
}

TEST_F(IntAbsNodeTest, application) {
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