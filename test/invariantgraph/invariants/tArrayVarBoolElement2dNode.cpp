#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayVarBoolElement2dNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarBoolElement2dNodeTest
    : public NodeTestBase<ArrayVarBoolElement2dNode> {
 public:
  VarNodeId x00;
  VarNodeId x01;
  VarNodeId x10;
  VarNodeId x11;

  VarNodeId idx1;
  VarNodeId idx2;
  VarNodeId y;

  void SetUp() override {
    NodeTestBase::SetUp();
    x00 = createBoolVar("x00");
    x01 = createBoolVar("x01");
    x10 = createBoolVar("x10");
    x11 = createBoolVar("x11");
    idx1 = createIntVar(1, 2, "idx1");
    idx2 = createIntVar(1, 2, "idx2");
    y = createBoolVar("y");

    fznparser::BoolVarArray argMatrix("");
    argMatrix.append(boolVar(x00));
    argMatrix.append(boolVar(x01));
    argMatrix.append(boolVar(x10));
    argMatrix.append(boolVar(x11));

    _model->addConstraint(fznparser::Constraint(
        "array_var_bool_element2d_nonshifted_flat",
        std::vector<fznparser::Arg>{
            fznparser::IntArg{intVar(idx1)}, fznparser::IntArg{intVar(idx2)},
            argMatrix, fznparser::BoolArg{boolVar(y)}, fznparser::IntArg{2},
            fznparser::IntArg{1}, fznparser::IntArg{1}}));

    makeInvNode(_model->constraints().front());
  }
};

TEST_F(ArrayVarBoolElement2dNodeTest, construction) {
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

TEST_F(ArrayVarBoolElement2dNodeTest, application) {
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

  // x00, x01, x10, x11, idx1, idx2
  EXPECT_EQ(solver.searchVars().size(), 6);

  // x00, x01, x10, x11, idx1, idx2, and y
  EXPECT_EQ(solver.numVars(), 7);

  // element2dVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayVarBoolElement2dNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()), propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 4);
  for (const auto& inputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputs;
  inputs.emplace_back(varId(invNode().idx1()));
  inputs.emplace_back(varId(invNode().idx2()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputs.emplace_back(varId(varNodeId));
    solver.updateBounds(varId(varNodeId), 0, 3, true);
  }
  solver.close();
  std::vector<Int> values(inputs.size(), 0);

  for (values.at(0) = solver.lowerBound(inputs.at(0));
       values.at(0) <= solver.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = solver.lowerBound(inputs.at(1));
         values.at(1) <= solver.upperBound(inputs.at(1)); ++values.at(1)) {
      for (values.at(2) = solver.lowerBound(inputs.at(2));
           values.at(2) <= solver.upperBound(inputs.at(2)); ++values.at(2)) {
        for (values.at(3) = solver.lowerBound(inputs.at(3));
             values.at(3) <= solver.upperBound(inputs.at(3)); ++values.at(3)) {
          for (values.at(4) = solver.lowerBound(inputs.at(4));
               values.at(4) <= solver.upperBound(inputs.at(4));
               ++values.at(4)) {
            for (values.at(5) = solver.lowerBound(inputs.at(5));
                 values.at(5) <= solver.upperBound(inputs.at(5));
                 ++values.at(5)) {
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

              const Int index = 2 + (row * 2 + col);

              EXPECT_EQ(actual, values.at(index));
            }
          }
        }
      }
    }
  }
}

}  // namespace atlantis::testing