#include "../nodeTestBase.hpp"
#include "invariantgraph/fzn/array_int_element2d.hpp"
#include "invariantgraph/invariantNodes/arrayIntElement2dNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayIntElement2dNodeTest : public NodeTestBase<ArrayIntElement2dNode> {
 public:
  std::vector<std::vector<Int>> parMatrix{std::vector<Int>{0, 1},
                                          std::vector<Int>{2, 3}};

  VarNodeId idx1 = NULL_NODE_ID;
  VarNodeId idx2 = NULL_NODE_ID;
  VarNodeId outputVar = NULL_NODE_ID;

  void SetUp() override {
    NodeTestBase::SetUp();
    addFznVar(1, 2, "idx1");
    addFznVar(1, 2, "idx2");
    addFznVar(0, 3, "outputVar");

    fznparser::IntVarArray argMatrix("");
    for (const auto& row : parMatrix) {
      for (const auto& elem : row) {
        argMatrix.append(elem);
      }
    }

    _model->addConstraint(fznparser::Constraint(
        "array_int_element2d_nonshifted_flat",
        std::vector<fznparser::Arg>{
            fznparser::IntArg{intVar("idx1")},
            fznparser::IntArg{intVar("idx2")}, argMatrix,
            fznparser::IntArg{intVar("outputVar")},
            fznparser::IntArg{static_cast<Int>(parMatrix.size())},
            fznparser::IntArg{1}, fznparser::IntArg{1}}));

    fzn::array_int_element2d(*_invariantGraph, _model->constraints().front());

    idx1 = varNodeId("idx1");
    idx2 = varNodeId("idx2");
    outputVar = varNodeId("outputVar");
  }
};

TEST_F(ArrayIntElement2dNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVar);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
}

TEST_F(ArrayIntElement2dNodeTest, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // idx1, idx2
  EXPECT_EQ(solver.searchVars().size(), 2);

  // idx1, idx2, and outputVar
  EXPECT_EQ(solver.numVars(), 3);

  // element2dVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayIntElement2dNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()),
            propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputs;
  inputs.emplace_back(varId(invNode().idx1()));
  inputs.emplace_back(varId(invNode().idx2()));
  solver.close();
  std::vector<Int> values(inputs.size(), 0);

  for (values.at(0) = solver.lowerBound(inputs.at(0));
       values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = solver.lowerBound(inputs.at(1));
         values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
      solver.beginMove();
      for (size_t i = 0; i < inputs.size(); ++i) {
        solver.setValue(inputs.at(i), values.at(i));
      }
      solver.endMove();

      solver.beginProbe();
      solver.query(outputId);
      solver.endProbe();

      const Int actual = solver.currentValue(outputId);
      const Int row = values.at(0) - 1;  // offset of 1
      const Int col = values.at(1) - 1;  // offset of 1

      EXPECT_EQ(actual, parMatrix.at(row).at(col));
    }
  }
}

}  // namespace atlantis::testing