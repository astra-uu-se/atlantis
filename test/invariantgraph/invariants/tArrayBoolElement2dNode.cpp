#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayBoolElement2dNode.hpp"
#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"
#include "propagation/propagationEngine.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayBoolElement2dNodeTest : public NodeTestBase<ArrayIntElement2dNode> {
 public:
  std::vector<std::vector<bool>> parMatrix{std::vector<bool>{true, false},
                                           std::vector<bool>{false, true}};

  VarNodeId idx1;
  VarNodeId idx2;
  VarNodeId y;

  void SetUp() override {
    NodeTestBase::SetUp();
    idx1 = createIntVar(1, 2, "idx1");
    idx2 = createIntVar(1, 2, "idx2");
    y = createBoolVar("y");

    fznparser::BoolVarArray argMatrix("");
    for (const auto& row : parMatrix) {
      for (const auto& elem : row) {
        argMatrix.append(elem);
      }
    }

    _model->addConstraint(fznparser::Constraint(
        "array_bool_element2d_nonshifted_flat",
        std::vector<fznparser::Arg>{
            fznparser::IntArg{intVar(idx1)}, fznparser::IntArg{intVar(idx2)},
            argMatrix, fznparser::BoolArg{boolVar(y)},
            fznparser::IntArg{static_cast<Int>(parMatrix.size())},
            fznparser::IntArg{1}, fznparser::IntArg{1}}));

    makeOtherInvNode<ArrayBoolElement2dNode>(_model->constraints().front());
  }
};

TEST_F(ArrayBoolElement2dNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), y);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
}

TEST_F(ArrayBoolElement2dNodeTest, application) {
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

  // idx1, idx2
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // idx1, idx2, and y
  EXPECT_EQ(engine.numVariables(), 3);

  // element2dVar
  EXPECT_EQ(engine.numInvariants(), 1);
}

TEST_F(ArrayBoolElement2dNodeTest, propagation) {
  propagation::PropagationEngine engine;
  engine.open();
  addInputVarsToEngine(engine);
  invNode().registerOutputVariables(*_invariantGraph, engine);
  invNode().registerNode(*_invariantGraph, engine);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputs;
  inputs.emplace_back(varId(invNode().idx1()));
  inputs.emplace_back(varId(invNode().idx2()));
  engine.close();
  std::vector<Int> values(inputs.size(), 0);

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
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

      EXPECT_EQ(actual == 0, parMatrix.at(row).at(col));
    }
  }
}

}  // namespace atlantis::testing