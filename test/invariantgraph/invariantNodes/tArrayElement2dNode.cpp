#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElement2dNodeTestFixture : public NodeTestBase<ArrayElement2dNode> {
 public:
  std::vector<std::vector<Int>> parMatrix{std::vector<Int>{-2, -1},
                                          std::vector<Int>{1, 0}};

  std::string idx1Var{"idx1"};
  std::string idx2Var{"idx2"};
  std::string outputVar{"output"};

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

  void SetUp() {
    NodeTestBase::SetUp();
    retrieveIntVarNode(
        idx1Offset,
        shouldBeSubsumed() || idx1ShouldBeReplaced()
            ? idx1Offset
            : (idx1Offset + static_cast<Int>(parMatrix.size()) - 1),
        idx1Var);
    retrieveIntVarNode(
        idx2Offset,
        shouldBeSubsumed() || idx2ShouldBeReplaced()
            ? idx2Offset
            : (idx1Offset + static_cast<Int>(parMatrix.front().size()) - 1),
        idx2Var);

    if (isIntElement()) {
      // int version of element
      Int lb = std::numeric_limits<Int>::max();
      Int ub = std::numeric_limits<Int>::max();
      for (const auto& row : parMatrix) {
        const Int mn = std::ranges::min(row);
        const Int mx = std::ranges::max(row);
        lb = std::min(lb, mn);
        ub = std::max(ub, mx);
      }

      retrieveIntVarNode(lb, ub, outputVar);
      createInvariantNode(*_invariantGraph, varNodeId(idx1Var),
                          varNodeId(idx2Var),
                          std::vector<std::vector<Int>>{parMatrix},
                          varNodeId(outputVar), idx1Offset, idx2Offset);
    } else {
      // bool version of element
      retrieveBoolVarNode(outputVar);
      std::vector<std::vector<bool>> boolMatrix;
      boolMatrix.reserve(parMatrix.size());
      for (const auto& row : parMatrix) {
        boolMatrix.emplace_back();
        for (const auto val : row) {
          boolMatrix.back().emplace_back(intParToBool(val));
        }
      }
      createInvariantNode(*_invariantGraph, varNodeId(idx1Var),
                          varNodeId(idx2Var), std::move(boolMatrix),
                          varNodeId(outputVar), idx1Offset, idx2Offset);
    }
  }
};

TEST_P(ArrayElement2dNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  invNode().updateState();
  if (shouldBeSubsumed()) {
    EXPECT_EQ(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_TRUE(varNode(outputVar).isFixed());
    const Int expected = computeOutput();
    const Int actual = varNode(outputVar).lowerBound();
    EXPECT_EQ(expected, actual);
  } else {
    EXPECT_NE(invNode().state(), InvariantNodeState::SUBSUMED);
    EXPECT_FALSE(varNode(outputVar).isFixed());
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

  VarNode outputNode = varNode(outputVar);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual =
        parVal(parMatrix.at(varNode(idx1Var).lowerBound() - idx1Offset)
                   .at(varNode(idx2Var).lowerBound() - idx2Offset));
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputVar), propagation::NULL_ID);
  const propagation::VarViewId outputId = varId(outputVar);

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& idx : std::array<std::string, 2>{idx1Var, idx2Var}) {
    if (!varNode(idx).isFixed()) {
      EXPECT_NE(varId(idx), propagation::NULL_ID);
      inputVarIds.emplace_back(varId(idx));
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

RC_GTEST_PROP(ArrayElement2dNodeTest, RapidCheck, ()) {
  /*
  const size_t numRows = *rc::gen::inRange<size_t>(1, 4);
  const size_t numCols = *rc::gen::inRange<size_t>(1, 4);

  idx1Offset = *rc::gen::inRange<Int>(
      std::numeric_limits<Int>::min() + static_cast<Int>(numRows) + 1,
      std::numeric_limits<Int>::max() - static_cast<Int>(numRows));
  idx2Offset = *rc::gen::inRange<Int>(
      std::numeric_limits<Int>::min() + static_cast<Int>(numCols) + 1,
      std::numeric_limits<Int>::max() - static_cast<Int>(numCols));

  parMatrix.resize(numRows);
  for (size_t i = 0; i < numRows; ++i) {
    params.at(i).resize(numCols);
    for (size_t j = 0; j < numCols; ++j) {
      params.at(i).at(j) = *rc::gen::arbitrary<Int>();
    }
  }



  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(varId(outputVar)) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(varId(idx1Var), idx1Dist(gen));
      }
      if (randBool()) {
        _solver->setValue(varId(idx2Var), idx2Dist(gen));
      }

      _solver->endMove();

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      _solver->query(varId(outputVar));
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(_solver->currentValue(varId(outputVar)) == computeOutput());
    }
    RC_ASSERT(_solver->committedValue(varId(outputVar)) == computeOutput(true));
  }
  */
}

}  // namespace atlantis::testing
