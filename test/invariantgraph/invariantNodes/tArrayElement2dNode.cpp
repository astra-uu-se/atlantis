#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElement2dNodeTestFixture : public NodeTestBase<ArrayElement2dNode> {
 public:
  std::vector<std::vector<Int>> parMatrix{std::vector<Int>{-2, -1},
                                          std::vector<Int>{0, 1}};

  Var idx1Var{NULL_NODE_ID, "idx1"};
  Var idx2Var{NULL_NODE_ID, "idx2"};
  Var outputVar{NULL_NODE_ID, "output"};

  Int idx1Offset{1};
  Int idx2Offset{1};

  [[nodiscard]] static bool intParToBool(const Int val) {
    return std::abs(val) % 2 == 0;
  }

  [[nodiscard]] Int parVal(const Int val) const {
    return isIntElement() ? val : intParToBool(val) ? 0 : 1;
  }

  [[nodiscard]] bool isIntElement() const { return _paramData.data <= 1; }
  [[nodiscard]] bool idx1ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }
  [[nodiscard]] bool idx2ShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  Int computeOutput(bool isRegistered = false) {
    if (isRegistered) {
      const Int row =
          (varNode(idx1Var).isFixed() ? varNode(idx1Var).lowerBound()
                                      : _solver->currentValue(varId(idx1Var))) -
          idx1Offset;
      const Int col =
          (varNode(idx2Var).isFixed() ? varNode(idx2Var).lowerBound()
                                      : _solver->currentValue(varId(idx2Var))) -
          idx2Offset;
      return parVal(parMatrix.at(row).at(col));
    }
    const Int row = varNode(idx1Var).lowerBound() - idx1Offset;
    const Int col = varNode(idx2Var).lowerBound() - idx2Offset;
    return parVal(parMatrix.at(row).at(col));
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    idx1Var.id = retrieveIntVarNode(
        idx1Offset,
        shouldBeSubsumed() || idx1ShouldBeReplaced()
            ? idx1Offset
            : (idx1Offset + static_cast<Int>(parMatrix.size()) - 1),
        idx1Var.identifier);
    idx2Var.id = retrieveIntVarNode(
        idx2Offset,
        shouldBeSubsumed() || idx2ShouldBeReplaced()
            ? idx2Offset
            : (idx1Offset + static_cast<Int>(parMatrix.front().size()) - 1),
        idx2Var.identifier);

    if (isIntElement()) {
      // int version of element
      outputVar.id = retrieveIntVarNode(-2, 1, outputVar.identifier);
      createInvariantNode(*_invariantGraph, idx1Var.id, idx2Var.id,
                          std::vector<std::vector<Int>>{parMatrix},
                          outputVar.id, idx1Offset, idx2Offset);
    } else {
      // bool version of element
      outputVar.id = retrieveBoolVarNode(outputVar.identifier);
      std::vector<std::vector<bool>> boolMatrix;
      boolMatrix.reserve(parMatrix.size());
      for (const auto& row : parMatrix) {
        boolMatrix.emplace_back();
        for (const auto val : row) {
          boolMatrix.back().emplace_back(intParToBool(val));
        }
      }
      createInvariantNode(*_invariantGraph, idx1Var.id, idx2Var.id,
                          std::move(boolMatrix), outputVar.id, idx1Offset,
                          idx2Offset);
    }
  }
};

TEST_P(ArrayElement2dNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar.id).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar.id).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar.id).isFixed());
  }
}

TEST_P(ArrayElement2dNodeTestFixture, replace) {
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

TEST_P(ArrayElement2dNodeTestFixture, propagation) {
  _invariantGraph->construct();
  _invariantGraph->close();

  VarNode outputNode = varNode(outputVar.identifier);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual =
        parVal(parMatrix.at(varNode(idx1Var.id).lowerBound() - idx1Offset)
                   .at(varNode(idx2Var.id).lowerBound() - idx2Offset));
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputVar.identifier), propagation::NULL_ID);
  const propagation::VarViewId outputId = varId(outputVar.identifier);

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& idxVarNodeId :
       std::array<VarNodeId, 2>{idx1Var.id, idx2Var.id}) {
    if (!varNode(idxVarNodeId).isFixed()) {
      EXPECT_NE(varId(idxVarNodeId), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(idxVarNodeId));
    }
  }

  std::vector<Int> inputVals = makeInputVals(inputVarIds);
  EXPECT_FALSE(inputVals.empty());

  while (increaseNextVal(inputVarIds, inputVals) >= 0) {
    _solver->beginMove();
    setVarVals(inputVarIds, inputVals);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(outputId);
    _solver->endProbe();

    expectVarVals(inputVarIds, inputVals);

    const Int actual = _solver->currentValue(outputId);
    const Int expected = computeOutput(true);

    EXPECT_EQ(actual, expected);
  }
}

INSTANTIATE_TEST_CASE_P(
    ArrayElement2dNodeTest, ArrayElement2dNodeTestFixture,
    ::testing::Values(ParamData{0}, ParamData{InvariantNodeAction::SUBSUME, 0},
                      ParamData{InvariantNodeAction::REPLACE, 0},
                      ParamData{InvariantNodeAction::REPLACE, 1}, ParamData{2},
                      ParamData{InvariantNodeAction::SUBSUME, 2},
                      ParamData{InvariantNodeAction::REPLACE, 2},
                      ParamData{InvariantNodeAction::REPLACE, 3}));

}  // namespace atlantis::testing
