#include "../nodeTestBase.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayElement2dNodeTestFixture : public NodeTestBase<ArrayElement2dNode> {
 protected:
  std::vector<std::vector<Int>> intMatrix{std::vector<Int>{-2, -1, 0},
                                          std::vector<Int>{1, 2, 0}};
  std::vector<std::vector<bool>> boolMatrix{{true, false, true},
                                            {false, true, false}};

  Var rowIdxVar{"idx1", std::vector<Int>{}, true};
  Var colIdxVar{"idx2", std::vector<Int>{}, true};
  Var outputVar{"output", std::vector<Int>{}, true};

  Int rowOffset{-5};
  Int colOffset{5};

  [[nodiscard]] Int computeOutput(const Int rowIdx, const Int colIdx) const {
    return isIntElement()
               ? intMatrix.at(rowIdx - rowOffset).at(colIdx - colOffset)
           : boolMatrix.at(rowIdx - rowOffset).at(colIdx - colOffset) ? 0
                                                                      : 1;
  }

  [[nodiscard]] bool isIntElement() const { return _paramData.data <= 1; }
  [[nodiscard]] bool rowIndexShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 0 || _paramData.data == 2);
  }
  [[nodiscard]] bool colIndexShouldBeReplaced() const {
    return shouldBeReplaced() && (_paramData.data == 1 || _paramData.data == 3);
  }

  Int computeOutput(const bool isRegistered = false) {
    if (isRegistered) {
      const Int row = varNode(rowIdxVar).isFixed()
                          ? varNode(rowIdxVar).lowerBound()
                          : _solver->currentValue(varId(rowIdxVar));
      const Int col = varNode(colIdxVar).isFixed()
                          ? varNode(colIdxVar).lowerBound()
                          : _solver->currentValue(varId(colIdxVar));
      return computeOutput(row, col);
    }
    const Int row = varNode(rowIdxVar).lowerBound();
    const Int col = varNode(colIdxVar).lowerBound();
    return computeOutput(row, col);
  }

  void SetUp() override {
    NodeTestBase::SetUp();
    if (shouldBeSubsumed() || rowIndexShouldBeReplaced()) {
      rowIdxVar.domain = std::vector<Int>{rowOffset};
    } else {
      rowIdxVar.domain = std::pair<Int, Int>{
          rowOffset, rowOffset + static_cast<Int>(intMatrix.size()) - 1};
    }
    retrieveIntVarNode(rowIdxVar);

    if (shouldBeSubsumed() || colIndexShouldBeReplaced()) {
      colIdxVar.domain = std::vector<Int>{colOffset};
    } else {
      colIdxVar.domain = std::pair<Int, Int>{
          colOffset,
          colOffset + static_cast<Int>(intMatrix.front().size()) - 1};
    }
    retrieveIntVarNode(colIdxVar);

    if (isIntElement()) {
      // int version of element
      Int lb = std::numeric_limits<Int>::max();
      Int ub = std::numeric_limits<Int>::min();
      for (const auto& row : intMatrix) {
        const Int mn = std::ranges::min(row);
        const Int mx = std::ranges::max(row);
        lb = std::min(lb, mn);
        ub = std::max(ub, mx);
      }
      outputVar.domain = std::pair<Int, Int>{lb - 1, ub + 1};
      retrieveIntVarNode(outputVar);
      createInvariantNode(*_invariantGraph, varNodeId(rowIdxVar),
                          varNodeId(colIdxVar),
                          std::vector<std::vector<Int>>{intMatrix},
                          varNodeId(outputVar), rowOffset, colOffset);
    } else {
      // bool version of element
      outputVar.domain = std::vector<Int>{0, 1};
      retrieveBoolVarNode(outputVar);
      createInvariantNode(*_invariantGraph, varNodeId(rowIdxVar),
                          varNodeId(colIdxVar),
                          std::vector<std::vector<bool>>(boolMatrix),
                          varNodeId(outputVar), rowOffset, colOffset);
    }
  }
};

TEST_P(ArrayElement2dNodeTestFixture, updateState) {
  EXPECT_EQ(invNode().state(), InvariantNodeState::ACTIVE);
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
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
  _invariantGraph->constraintSolver().fixPoint();
  _invariantGraph->updateDomains();
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
  _invariantGraph->close();
  _solverMapping =
      std::make_shared<SolverMapping>(_invariantGraph->construct(*_solver));

  const VarNode outputNode = varNode(outputVar);

  if (outputNode.isFixed()) {
    const Int expected = outputNode.lowerBound();
    const Int actual = computeOutput(varNode(rowIdxVar).lowerBound(),
                                     varNode(colIdxVar).lowerBound());
    EXPECT_EQ(expected, actual);
    return;
  }

  EXPECT_NE(varId(outputVar), propagation::NULL_ID);
  const propagation::VarViewId outputId = varId(outputVar);

  std::vector<propagation::VarViewId> inputVarIds;
  for (const auto& idx :
       std::array<std::string, 2>{rowIdxVar.identifier, colIdxVar.identifier}) {
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

INSTANTIATE_TEST_SUITE_P(
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
