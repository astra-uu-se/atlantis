#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/fzn/array_var_int_element2d.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElement2dNodeTestFixture
    : public NodeTestBase<ArrayVarElement2dNode> {
 public:
  std::vector<std::vector<VarNodeId>> varMatrix;

  VarNodeId idx1{NULL_NODE_ID};
  VarNodeId idx2{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  bool isIntElement() const { return _mode <= 2; }
  bool idx1ShouldBeReplaced() const { return _mode == 1 || _mode == 4; }
  bool idx2ShouldBeReplaced() const { return _mode == 2 || _mode == 5; }
  bool shouldBeReplaced() const {
    return idx1ShouldBeReplaced() || idx2ShouldBeReplaced();
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (isIntElement()) {
      varMatrix = {
          {retrieveIntVarNode(-2, -1, "x00"), retrieveIntVarNode(-1, 0, "x01")},
          {retrieveIntVarNode(0, 1, "x10"), retrieveIntVarNode(1, 2, "x11")}};
      output = retrieveIntVarNode(-2, 2, "output");
    } else {
      varMatrix = {{retrieveBoolVarNode("x00"), retrieveBoolVarNode("x01")},
                   {retrieveBoolVarNode("x10"), retrieveBoolVarNode("x11")}};
      output = retrieveBoolVarNode("output");
    }

    idx1 = retrieveIntVarNode(
        offsetIdx1,
        idx1ShouldBeReplaced()
            ? offsetIdx1
            : (offsetIdx1 + static_cast<Int>(varMatrix.size()) - 1),
        "idx1");
    idx2 = retrieveIntVarNode(
        offsetIdx2,
        idx2ShouldBeReplaced()
            ? offsetIdx2
            : (offsetIdx2 + static_cast<Int>(varMatrix.front().size()) - 1),
        "idx2");

    createInvariantNode(idx1, idx2,
                        std::vector<std::vector<VarNodeId>>{varMatrix}, output,
                        offsetIdx1, offsetIdx2);
  }
};

TEST_P(ArrayVarElement2dNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  size_t i = 0;
  for (const auto& row : varMatrix) {
    for (const auto& varNodeId : row) {
      EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(i), varNodeId);
      ++i;
    }
  }
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), i);
}

TEST_P(ArrayVarElement2dNodeTestFixture, application) {
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

  // x00, x01, x10, x11, idx1, idx2, and output
  EXPECT_EQ(solver.numVars(), 7);

  // element2dVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_P(ArrayVarElement2dNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState(*_invariantGraph);
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced(*_invariantGraph));
    EXPECT_TRUE(invNode().replace(*_invariantGraph));
    invNode().deactivate(*_invariantGraph);
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced(*_invariantGraph));
  }
}

TEST_P(ArrayVarElement2dNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->apply(solver);

  const propagation::VarId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarId> inputVars;
  std::vector<Int> inputVals;

  for (const auto& nId : std::array<VarNodeId, 2>{idx1, idx2}) {
    inputVars.emplace_back(varNode(nId).isFixed() ? propagation::NULL_ID
                                                  : varId(nId));
    inputVals.emplace_back(inputVars.back() == propagation::NULL_ID
                               ? varNode(nId).lowerBound()
                               : solver.lowerBound(inputVars.back()));
  }
  for (const auto& row : varMatrix) {
    for (const auto& nId : row) {
      inputVars.emplace_back(varNode(nId).isFixed() ? propagation::NULL_ID
                                                    : varId(nId));
      inputVals.emplace_back(inputVars.back() == propagation::NULL_ID
                                 ? varNode(nId).lowerBound()
                                 : solver.lowerBound(inputVars.back()));
    }
  }

  EXPECT_EQ(inputVars.size(), inputVals.size());

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

    const Int index =
        2 + (row * static_cast<Int>(varMatrix.front().size()) + col);

    EXPECT_EQ(actual, inputVals.at(index));
  }
}

INSTANTIATE_TEST_CASE_P(ArrayVarElement2dNodeTest,
                        ArrayVarElement2dNodeTestFixture,
                        ::testing::Values(0, 1, 2, 3, 4, 5));

}  // namespace atlantis::testing
