#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class IntModNodeTest : public NodeTestBase<IntModNode> {
 public:
  VarNodeId numerator{NULL_NODE_ID};
  VarNodeId denominator{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    numerator = retrieveIntVarNode(0, 6, "numerator");
    denominator = retrieveIntVarNode(1, 10, "denominator");
    output = retrieveIntVarNode(0, 10, "output");

    createInvariantNode(numerator, denominator, output);
  }
};

TEST_F(IntModNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().numerator(), numerator);
  EXPECT_EQ(invNode().denominator(), denominator);
  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);
}

TEST_F(IntModNodeTest, application) {
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

  // numerator and denominator
  EXPECT_EQ(solver.searchVars().size(), 2);

  // numerator, denominator and output
  EXPECT_EQ(solver.numVars(), 3);

  // intMod
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(IntModNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  std::vector<propagation::VarId> inputVars;
  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
    inputVars.emplace_back(varId(inputVarNodeId));
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());
  EXPECT_EQ(inputVars.size(), 2);

  solver.close();

  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    if (inputVals.at(1) != 0) {
      const Int actual = solver.currentValue(outputId);
      const Int expected = inputVals.at(0) % inputVals.at(1);
      EXPECT_EQ(actual, expected);
    }
  }
}

}  // namespace atlantis::testing
