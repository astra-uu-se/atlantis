#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"

class ArrayVarBoolElementNodeTest
    : public NodeTestBase<invariantgraph::ArrayVarBoolElementNode> {
 public:
  invariantgraph::VarNodeId a;
  invariantgraph::VarNodeId b;
  invariantgraph::VarNodeId c;

  invariantgraph::VarNodeId idx;
  invariantgraph::VarNodeId y;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createBoolVar("a");
    b = createBoolVar("b");
    c = createBoolVar("c");
    idx = createIntVar(0, 10, "idx");
    y = createBoolVar("y");

    fznparser::BoolVarArray inputs("");
    inputs.append(boolVar(a));
    inputs.append(boolVar(b));
    inputs.append(boolVar(c));

    _model->addConstraint(fznparser::Constraint(
        "array_var_bool_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(idx)}, inputs,
                                    fznparser::BoolArg{boolVar(y)}}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayVarBoolElementNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().b(), idx);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), y);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 3);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(0), a);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(1), b);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(2), c);
}

TEST_F(ArrayVarBoolElementNodeTest, application) {
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
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), NULL_ID);
  const VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<VarId> inputs;
  inputs.emplace_back(varId(invNode().staticInputVarNodeIds().front()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputs.emplace_back(varId(varNodeId));
    engine.updateBounds(varId(varNodeId), 0, 10, true);
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