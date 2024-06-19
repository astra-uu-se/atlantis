#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayBoolElement2dNodeTest : public NodeTestBase<ArrayElement2dNode> {
 public:
  std::vector<std::vector<bool>> parMatrix{std::vector<bool>{true, false},
                                           std::vector<bool>{false, true}};

  VarNodeId idx1{NULL_NODE_ID};
  VarNodeId idx2{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  void SetUp() override {
    NodeTestBase::SetUp();
    idx1 = retrieveIntVarNode(1, 2, "idx1");
    idx2 = retrieveIntVarNode(1, 2, "idx2");
    output = retrieveBoolVarNode("output");

    std::vector<std::vector<bool>> inputMatrix;
    inputMatrix.reserve(parMatrix.size());
    for (const auto& row : parMatrix) {
      inputMatrix.emplace_back(row);
    }

    createInvariantNode(idx1, idx2, std::move(inputMatrix), output, offsetIdx1,
                        offsetIdx2);
  }
};

TEST_F(ArrayBoolElement2dNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 0);
}

TEST_F(ArrayBoolElement2dNodeTest, application) {
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

  // idx1, idx2, and output
  EXPECT_EQ(solver.numVars(), 3);

  // element2dVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayBoolElement2dNodeTest, replace1) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  _invariantGraph->varNode(idx1).fixToValue(Int{1});
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(ArrayBoolElement2dNodeTest, replace2) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  _invariantGraph->varNode(idx2).fixToValue(Int{1});
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
  EXPECT_TRUE(invNode().replace(*_invariantGraph));
  invNode().deactivate(*_invariantGraph);
  EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
}

TEST_F(ArrayBoolElement2dNodeTest, propagation) {
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

  std::vector<propagation::VarId> inputVars;
  inputVars.emplace_back(varId(invNode().idx1()));
  inputVars.emplace_back(varId(invNode().idx2()));
  solver.close();
  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int row = inputVals.at(0) - offsetIdx1;
    const Int col = inputVals.at(1) - offsetIdx2;

    EXPECT_EQ(actual == 0, parMatrix.at(row).at(col));
  }
}

}  // namespace atlantis::testing
