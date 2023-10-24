#include "../nodeTestBase.hpp"
#include "invariantgraph/invariantNodes/arrayVarIntElement2dNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarIntElement2dNodeTest
    : public NodeTestBase<ArrayVarIntElement2dNode> {
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

TEST_F(ArrayVarIntElement2dNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  for (const auto& staticInputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(staticInputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 4);
  for (const auto& dynamicInputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(dynamicInputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId = varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputs;
  inputs.emplace_back(varId(invNode().idx1()));
  inputs.emplace_back(varId(invNode().idx2()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputs.emplace_back(varId(varNodeId));
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