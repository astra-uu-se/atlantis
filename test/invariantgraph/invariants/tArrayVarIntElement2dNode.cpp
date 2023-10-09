#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElement2dNode.hpp"

class ArrayVarIntElement2dNodeTest
    : public NodeTestBase<invariantgraph::ArrayVarIntElement2dNode> {
 public:
  invariantgraph::VarNodeId x00;
  invariantgraph::VarNodeId x01;
  invariantgraph::VarNodeId x10;
  invariantgraph::VarNodeId x11;

  invariantgraph::VarNodeId idx1;
  invariantgraph::VarNodeId idx2;
  invariantgraph::VarNodeId y;

  void SetUp() override {
    NodeTestBase::SetUp();
    x00 = createIntVar(3, 10, "x00");
    x01 = createIntVar(2, 11, "x01");
    x10 = createIntVar(1, 9, "x10");
    x11 = createIntVar(3, 5, "x11");
    idx1 = createIntVar(1, 2, "idx1");
    idx2 = createIntVar(1, 2, "idx2");
    y = createIntVar(0, 10, "y");

    fznparser::IntVarArray argMatrix("");
    argMatrix.append(intVar(x00));
    argMatrix.append(intVar(x01));
    argMatrix.append(intVar(x10));
    argMatrix.append(intVar(x11));

    _model->addConstraint(fznparser::Constraint(
        "array_var_int_element2d_nonshifted_flat",
        std::vector<fznparser::Arg>{
            fznparser::IntArg{intVar(idx1)}, fznparser::IntArg{intVar(idx2)},
            argMatrix, fznparser::IntArg{intVar(y)}, fznparser::IntArg{2},
            fznparser::IntArg{1}, fznparser::IntArg{1}}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayVarIntElement2dNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), y);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 4);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(0), x00);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(1), x01);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(2), x10);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(3), x11);
}

TEST_F(ArrayVarIntElement2dNodeTest, application) {
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

  // x00, x01, x10, x11, idx1, idx2
  EXPECT_EQ(engine.searchVariables().size(), 6);

  // x00, x01, x10, x11, idx1, idx2, and y
  EXPECT_EQ(engine.numVariables(), 7);

  // element2dVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayVarIntElement2dNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  for (const auto& staticInputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(staticInputVarNodeId), NULL_ID);
  }

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 4);
  for (const auto& dynamicInputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(dynamicInputVarNodeId), NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), NULL_ID);
  const VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<VarId> inputs;
  inputs.emplace_back(varId(invNode().idx1()));
  inputs.emplace_back(varId(invNode().idx2()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputs.emplace_back(varId(varNodeId));
  }

  engine.close();
  std::vector<Int> values(inputs.size(), 0);

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      for (values.at(2) = engine.lowerBound(inputs.at(2));
           values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
        for (values.at(3) = engine.lowerBound(inputs.at(3));
             values.at(3) <= engine.upperBound(inputs.at(3)); ++values.at(3)) {
          for (values.at(4) = engine.lowerBound(inputs.at(4));
               values.at(4) <= engine.upperBound(inputs.at(4));
               ++values.at(4)) {
            for (values.at(5) = engine.lowerBound(inputs.at(5));
                 values.at(5) <= engine.upperBound(inputs.at(5));
                 ++values.at(5)) {
              engine.beginMove();
              for (size_t i = 0; i < inputs.size(); ++i) {
                engine.setValue(inputs.at(i), values.at(i));
              }
              engine.endMove();

              engine.beginProbe();
              engine.query(outputId);
              engine.endProbe();
              const Int actual = engine.currentValue(outputId);
              const Int row = values.at(0) - 1;  // offset of 1
              const Int col = values.at(1) - 1;  // offset of 1

              const Int index = 2 + (row * 2 + col);

              EXPECT_EQ(actual, values.at(index));
            }
          }
        }
      }
    }
  }
}