#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElementNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarIntElementNodeTest : public NodeTestBase<ArrayVarIntElementNode> {
 public:
  VarNodeId a;
  VarNodeId b;
  VarNodeId c;

  VarNodeId idx;
  VarNodeId y;

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVar(3, 10, "a");
    b = createIntVar(2, 11, "b");
    c = createIntVar(1, 9, "c");
    idx = createIntVar(0, 10, "idx");
    y = createIntVar(0, 10, "y");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(a));
    inputs.append(intVar(b));
    inputs.append(intVar(c));

    _model->addConstraint(fznparser::Constraint(
        "array_var_int_element",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(idx)}, inputs,
                                    fznparser::IntArg{intVar(y)}}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayVarIntElementNodeTest, construction) {
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

TEST_F(ArrayVarIntElementNodeTest, application) {
  propagation::PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVariables(*_invariantGraph, engine);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
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

TEST_F(ArrayVarIntElementNodeTest, propagation) {
  propagation::PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 1);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputs;
  inputs.emplace_back(varId(invNode().staticInputVarNodeIds().front()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputs.emplace_back(varId(varNodeId));
  }

  const propagation::VarId input = inputs.front();
  engine.close();
  std::vector<Int> values(4, 0);

  for (values.at(0) = std::max(Int(1), engine.lowerBound(input));
       values.at(0) <= std::min(Int(3), engine.upperBound(input));
       ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      for (values.at(2) = engine.lowerBound(inputs.at(2));
           values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
        for (values.at(3) = engine.lowerBound(inputs.at(3));
             values.at(3) <= engine.upperBound(inputs.at(3)); ++values.at(3)) {
          engine.beginMove();
          for (size_t i = 0; i < inputs.size(); ++i) {
            engine.setValue(inputs.at(i), values.at(i));
          }
          engine.endMove();

          engine.beginProbe();
          engine.query(outputId);
          engine.endProbe();
          const Int actual = engine.currentValue(outputId);
          EXPECT_EQ(actual, values.at(values.at(0)));
        }
      }
    }
  }
}
}  // namespace atlantis::testing