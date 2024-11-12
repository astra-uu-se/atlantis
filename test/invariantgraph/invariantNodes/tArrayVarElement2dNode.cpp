#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElement2dNodeTestFixture
    : public NodeTestBase<ArrayVarElement2dNode> {
 public:
  std::vector<std::vector<VarNodeId>> varMatrixVarNodeIds;

  VarNodeId idx1VarNodeId{NULL_NODE_ID};
  VarNodeId idx2VarNodeId{NULL_NODE_ID};
  VarNodeId outputVarNodeId{NULL_NODE_ID};
  std::string outputIdentifier{"output"};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  bool isIntElement() const { return _paramData.data <= 2; }
  bool idx1ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }
  bool idx2ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (isIntElement()) {
      varMatrixVarNodeIds = {
          {retrieveIntVarNode(-2, -1, "x00"), retrieveIntVarNode(-1, 0, "x01")},
          {retrieveIntVarNode(0, 1, "x10"), retrieveIntVarNode(1, 2, "x11")}};
      outputVarNodeId = retrieveIntVarNode(-2, 2, outputIdentifier);
    } else {
      varMatrixVarNodeIds = {
          {retrieveBoolVarNode("x00"), retrieveBoolVarNode("x01")},
          {retrieveBoolVarNode("x10"), retrieveBoolVarNode("x11")}};
      outputVarNodeId = retrieveBoolVarNode(outputIdentifier);
    }

    idx1VarNodeId = retrieveIntVarNode(
        offsetIdx1,
        idx1ShouldBeReplaced()
            ? offsetIdx1
            : (offsetIdx1 + static_cast<Int>(varMatrixVarNodeIds.size()) - 1),
        "idx1");
    idx2VarNodeId = retrieveIntVarNode(
        offsetIdx2,
        idx2ShouldBeReplaced()
            ? offsetIdx2
            : (offsetIdx2 +
               static_cast<Int>(varMatrixVarNodeIds.front().size()) - 1),
        "idx2");

    createInvariantNode(
        *_invariantGraph, idx1VarNodeId, idx2VarNodeId,
        std::vector<std::vector<VarNodeId>>{varMatrixVarNodeIds},
        outputVarNodeId, offsetIdx1, offsetIdx2);
  }
};

TEST_P(ArrayVarElement2dNodeTestFixture, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1VarNodeId);
  EXPECT_EQ(invNode().idx2(), idx2VarNodeId);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), outputVarNodeId);

  size_t i = 0;
  for (const auto& row : varMatrixVarNodeIds) {
    for (const auto& varNodeId : row) {
      EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(i), varNodeId);
      ++i;
    }
  }
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), i);
}

TEST_P(ArrayVarElement2dNodeTestFixture, application) {
  _solver->open();
  addInputVarsToSolver();

  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars();
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode();
  _solver->close();

  // x00, x01, x10, x11, idx1VarNodeId, idx2VarNodeId
  EXPECT_EQ(_solver->searchVars().size(), 6);

  // x00, x01, x10, x11, idx1VarNodeId, idx2VarNodeId, and outputVarNodeId
  EXPECT_EQ(_solver->numVars(), 7);

  // element2dVar
  EXPECT_EQ(_solver->numInvariants(), 1);
}

TEST_P(ArrayVarElement2dNodeTestFixture, replace) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeReplaced()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
    EXPECT_TRUE(invNode().canBeReplaced());
    EXPECT_TRUE(invNode().replace());
    invNode().deactivate();
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
  } else {
    EXPECT_FALSE(invNode().canBeReplaced());
  }
}

TEST_P(ArrayVarElement2dNodeTestFixture, propagation) {
  propagation::Solver solver;
  _invariantGraph->construct();
  _invariantGraph->close();

  const propagation::VarViewId outputId = varId(outputIdentifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  for (const auto& idxVarNodeId :
       std::array<VarNodeId, 2>{idx1VarNodeId, idx2VarNodeId}) {
    inputVarIds.emplace_back(varNode(idxVarNodeId).isFixed()
                                 ? propagation::NULL_ID
                                 : varId(idxVarNodeId));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(idxVarNodeId).lowerBound()
                               : _solver->lowerBound(inputVarIds.back()));
  }
  for (const auto& row : varMatrixVarNodeIds) {
    for (const auto& nId : row) {
      inputVarIds.emplace_back(varNode(nId).isFixed() ? propagation::NULL_ID
                                                      : varId(nId));
      inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                                 ? varNode(nId).lowerBound()
                                 : _solver->lowerBound(inputVarIds.back()));
    }
  }

  EXPECT_EQ(inputVarIds.size(), inputVals.size());

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    const Int actual = _solver->currentValue(outputId);
    const Int row = inputVals.at(0) - offsetIdx1;
    const Int col = inputVals.at(1) - offsetIdx2;

    const Int index =
        2 + (row * static_cast<Int>(varMatrixVarNodeIds.front().size()) + col);

    EXPECT_EQ(actual, inputVals.at(index));
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayVarElement2dNodeTest, ArrayVarElement2dNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{InvariantNodeAction::REPLACE, 1}, ParamData{2},
                      ParamData{InvariantNodeAction::REPLACE, 2},
                      ParamData{InvariantNodeAction::REPLACE, 3}));

}  // namespace atlantis::testing
