#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

class ArrayIntMaximumTest
    : public NodeTestBase<invariantgraph::ArrayIntMaximumNode> {
 public:
  invariantgraph::VarNodeId x1;
  invariantgraph::VarNodeId x2;
  invariantgraph::VarNodeId x3;
  invariantgraph::VarNodeId o;

  void SetUp() override {
    NodeTestBase::SetUp();
    x1 = createIntVar(5, 10, "x1");
    x2 = createIntVar(0, 20, "x2");
    x3 = createIntVar(0, 20, "x3");
    o = createIntVar(0, 10, "o");

    fznparser::IntVarArray inputs("");
    inputs.append(intVar(x1));
    inputs.append(intVar(x2));
    inputs.append(intVar(x3));

    _model->addConstraint(fznparser::Constraint(
        "array_int_maximum",
        std::vector<fznparser::Arg>{fznparser::IntArg{intVar(o)}, inputs},
        std::vector<fznparser::Annotation>{
            definesVarAnnotation(identifier(o))}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayIntMaximumTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(0), x1);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(1), x2);
  EXPECT_EQ(invNode().staticInputVarNodeIds().at(2), x3);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), o);
}

TEST_F(ArrayIntMaximumTest, application) {
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

  EXPECT_EQ(engine.lowerBound(varId(o)), 5);
  EXPECT_EQ(engine.upperBound(varId(o)), 20);

  // x1, x2, and x3
  EXPECT_EQ(engine.searchVariables().size(), 3);

  // x1, x2 and o
  EXPECT_EQ(engine.numVariables(), 4);

  // maxSparse
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayIntMaximumTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);

  std::vector<VarId> inputs;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 3);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), NULL_ID);
    inputs.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), NULL_ID);
  const VarId outputId = varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputs.size(), 3);

  std::vector<Int> values(inputs.size());
  engine.close();

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
      for (values.at(2) = engine.lowerBound(inputs.at(2));
           values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
        engine.beginMove();
        for (size_t i = 0; i < inputs.size(); ++i) {
          engine.setValue(inputs.at(i), values.at(i));
        }
        engine.endMove();

        engine.beginProbe();
        engine.query(outputId);
        engine.endProbe();

        const Int expected = *std::max_element(values.begin(), values.end());
        const Int actual = engine.currentValue(outputId);
        EXPECT_EQ(expected, actual);
      }
    }
  }
}
