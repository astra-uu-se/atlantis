#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElement2dNodeTestFixture
    : public NodeTestBase<ArrayVarElement2dNode> {
 public:
  std::vector<std::vector<Var>> varMatrix;

  Var idx1Var{NULL_NODE_ID, "idx1"};
  Var idx2Var{NULL_NODE_ID, "idx2"};
  Var outputVar{NULL_NODE_ID, "output"};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  [[nodiscard]] bool isIntElement() const { return _paramData.data <= 2; }

  [[nodiscard]] bool idx1ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }

  [[nodiscard]] bool idx2ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  void SetUp() override {
    NodeTestBase::SetUp();

    if (isIntElement()) {
      varMatrix = {{makeIntVar(-2, -1, "x00"), makeIntVar(-1, 0, "x01")},
                   {makeIntVar(0, 1, "x10"), makeIntVar(1, 2, "x11")}};
      outputVar.id = retrieveIntVarNode(-2, 2, outputVar.identifier);
    } else {
      varMatrix = {{makeBoolVar("x00"), makeBoolVar("x01")},
                   {makeBoolVar("x10"), makeBoolVar("x11")}};
      outputVar.id = retrieveBoolVarNode(outputVar.identifier);
    }

    idx1Var.id = retrieveIntVarNode(
        offsetIdx1,
        idx1ShouldBeReplaced()
            ? offsetIdx1
            : (offsetIdx1 + static_cast<Int>(varMatrix.size()) - 1),
        idx1Var.identifier);
    idx2Var.id = retrieveIntVarNode(
        offsetIdx2,
        idx2ShouldBeReplaced()
            ? offsetIdx2
            : (offsetIdx2 + static_cast<Int>(varMatrix.front().size()) - 1),
        idx2Var.identifier);

    createInvariantNode(*_invariantGraph, idx1Var.id, idx2Var.id,
                        varNodeIds(varMatrix), outputVar.id, offsetIdx1,
                        offsetIdx2);
  }
};

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

  const propagation::VarViewId outputId = varId(outputVar.identifier);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  for (const auto& idxVarNodeId :
       std::array<VarNodeId, 2>{idx1Var.id, idx2Var.id}) {
    inputVarIds.emplace_back(varNode(idxVarNodeId).isFixed()
                                 ? propagation::NULL_ID
                                 : varId(idxVarNodeId));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(idxVarNodeId).lowerBound()
                               : _solver->lowerBound(inputVarIds.back()));
  }
  for (const auto& row : varMatrix) {
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
        2 + (row * static_cast<Int>(varMatrix.front().size()) + col);

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
