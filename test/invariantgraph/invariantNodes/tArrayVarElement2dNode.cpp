#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarElement2dNodeTestFixture
    : public NodeTestBase<ArrayVarElement2dNode> {
 public:
  std::vector<std::vector<std::string>> varMatrix;

  std::string idx1Var{"idx1"};
  std::string idx2Var{"idx2"};
  std::string outputVar{"output"};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  [[nodiscard]] bool isIntElement() const { return _paramData.data <= 2; }

  [[nodiscard]] bool idx1ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }

  [[nodiscard]] bool idx2ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  void SetUp() {
    NodeTestBase::SetUp();
    varMatrix = {{"x00", "x01"}, {"x10", "x11"}};
    if (isIntElement()) {
      retrieveIntVarNode(-2, -1, varMatrix.at(0).at(0));
      retrieveIntVarNode(-1, 0, varMatrix.at(0).at(1));
      retrieveIntVarNode(0, 1, varMatrix.at(1).at(0));
      retrieveIntVarNode(1, 2, varMatrix.at(1).at(1));
      retrieveIntVarNode(-2, 2, outputVar);
    } else {
      for (const auto& row : varMatrix) {
        for (const auto& identifier : row) {
          retrieveBoolVarNode(identifier);
        }
      }
      retrieveBoolVarNode(outputVar);
    }

    retrieveIntVarNode(
        offsetIdx1,
        idx1ShouldBeReplaced()
            ? offsetIdx1
            : (offsetIdx1 + static_cast<Int>(varMatrix.size()) - 1),
        idx1Var);
    retrieveIntVarNode(
        offsetIdx2,
        idx2ShouldBeReplaced()
            ? offsetIdx2
            : (offsetIdx2 + static_cast<Int>(varMatrix.front().size()) - 1),
        idx2Var);

    createInvariantNode(*_invariantGraph, varNodeId(idx1Var),
                        varNodeId(idx2Var), varNodeIds(varMatrix),
                        varNodeId(outputVar), offsetIdx1, offsetIdx2);
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

  const propagation::VarViewId outputId = varId(outputVar);
  EXPECT_NE(outputId, propagation::NULL_ID);

  std::vector<propagation::VarViewId> inputVarIds;
  std::vector<Int> inputVals;

  for (const auto& idx : std::array<std::string, 2>{idx1Var, idx2Var}) {
    inputVarIds.emplace_back(varNode(idx).isFixed() ? propagation::NULL_ID
                                                    : varId(idx));
    inputVals.emplace_back(inputVarIds.back() == propagation::NULL_ID
                               ? varNode(idx).lowerBound()
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
