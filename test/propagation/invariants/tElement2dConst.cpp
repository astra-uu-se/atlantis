#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class Element2dConstTest : public InvariantTest {
 protected:
  const size_t numRows = 4;
  const size_t numCols = 5;
  std::vector<std::pair<Int, Int>> offsets =
      cartesianProduct(std::vector<Int>{-10, 0, 10});
  std::vector<std::vector<Int>> matrix;

 public:
  void SetUp() override {
    const Int inputLb = std::numeric_limits<Int>::min();
    const Int inputUb = std::numeric_limits<Int>::max();
    std::uniform_int_distribution<Int> inputDist(inputLb, inputUb);

    InvariantTest::SetUp();
    matrix.resize(numRows, std::vector<Int>(numCols));
    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        matrix.at(i).at(j) = inputDist(gen);
      }
    }
  }

  void TearDown() override {
    InvariantTest::TearDown();
    matrix.clear();
  }

  [[nodiscard]] size_t zeroBasedRowIndex(const Int rowIndexVal,
                                         const Int rowOffset) const {
    EXPECT_LE(rowOffset, rowIndexVal);
    EXPECT_LT(rowIndexVal - rowOffset, static_cast<Int>(numRows));
    return rowIndexVal - rowOffset;
  }

  [[nodiscard]] size_t zeroBasedColIndex(const Int colIndexVal,
                                         const Int colOffset) const {
    EXPECT_LE(colOffset, colIndexVal);
    EXPECT_LT(colIndexVal - colOffset, static_cast<Int>(numCols));
    return colIndexVal - colOffset;
  }

  Int computeOutput(const Timestamp ts, const VarViewId rowIndex,
                    const VarViewId colIndex, const Int rowOffset,
                    const Int colOffset) {
    return computeOutput(_solver->value(ts, rowIndex),
                         _solver->value(ts, colIndex), rowOffset, colOffset);
  }

  Int computeOutput(const Int rowIndexVal, const Int colIndexVal,
                    const Int rowOffset, const Int colOffset) {
    return matrix.at(zeroBasedRowIndex(rowIndexVal, rowOffset))
        .at(zeroBasedColIndex(colIndexVal, colOffset));
  }
};

TEST_F(Element2dConstTest, UpdateBounds) {
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = static_cast<Int>(numRows) - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = static_cast<Int>(numCols) - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    _solver->open();
    const VarViewId rowIndex =
        _solver->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarViewId colIndex =
        _solver->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<Int>>(matrix), rowOffset, colOffset);
    _solver->close();

    for (Int minRowIndex = rowIndexLb; minRowIndex <= rowIndexUb;
         ++minRowIndex) {
      for (Int maxRowIndex = rowIndexUb; maxRowIndex >= minRowIndex;
           --maxRowIndex) {
        _solver->updateBounds(VarId(rowIndex), minRowIndex, maxRowIndex, false);
        for (Int minColIndex = colIndexLb; minColIndex <= colIndexUb;
             ++minColIndex) {
          for (Int maxColIndex = colIndexUb; maxColIndex >= minColIndex;
               --maxColIndex) {
            _solver->updateBounds(VarId(colIndex), minColIndex, maxColIndex,
                                  false);
            invariant.updateBounds(false);
            Int minVal = std::numeric_limits<Int>::max();
            Int maxVal = std::numeric_limits<Int>::min();
            for (Int rowIndexVal = minRowIndex; rowIndexVal <= maxRowIndex;
                 ++rowIndexVal) {
              for (Int colIndexVal = minColIndex; colIndexVal <= maxColIndex;
                   ++colIndexVal) {
                minVal =
                    std::min(minVal, computeOutput(rowIndexVal, colIndexVal,
                                                   rowOffset, colOffset));
                maxVal =
                    std::max(maxVal, computeOutput(rowIndexVal, colIndexVal,
                                                   rowOffset, colOffset));
              }
            }
            EXPECT_EQ(minVal, _solver->lowerBound(outputId));
            EXPECT_EQ(maxVal, _solver->upperBound(outputId));
          }
        }
      }
    }
  }
}

TEST_F(Element2dConstTest, Recompute) {
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = static_cast<Int>(numRows) - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = static_cast<Int>(numCols) - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    _solver->open();
    const VarViewId rowIndex =
        _solver->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarViewId colIndex =
        _solver->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<Int>>(matrix), rowOffset, colOffset);
    _solver->close();

    for (Int rowIndexVal = rowIndexLb; rowIndexVal <= rowIndexUb;
         ++rowIndexVal) {
      _solver->setValue(_solver->currentTimestamp(), rowIndex, rowIndexVal);
      EXPECT_EQ(_solver->value(_solver->currentTimestamp(), rowIndex),
                rowIndexVal);
      for (Int colIndexVal = colIndexLb; colIndexVal <= colIndexUb;
           ++colIndexVal) {
        _solver->setValue(_solver->currentTimestamp(), colIndex, colIndexVal);
        EXPECT_EQ(_solver->value(_solver->currentTimestamp(), colIndex),
                  colIndexVal);

        const Int expectedOutput =
            computeOutput(_solver->currentTimestamp(), rowIndex, colIndex,
                          rowOffset, colOffset);
        invariant.recompute(_solver->currentTimestamp());
        EXPECT_EQ(_solver->value(_solver->currentTimestamp(), rowIndex),
                  rowIndexVal);

        EXPECT_EQ(expectedOutput,
                  _solver->value(_solver->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(Element2dConstTest, NotifyInputChanged) {
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = static_cast<Int>(numRows) - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = static_cast<Int>(numCols) - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    _solver->open();
    const VarViewId rowIndex =
        _solver->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarViewId colIndex =
        _solver->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<Int>>(matrix), rowOffset, colOffset);
    _solver->close();

    Timestamp ts = _solver->currentTimestamp();

    for (Int rowIndexVal = rowIndexLb; rowIndexVal <= rowIndexUb;
         ++rowIndexVal) {
      for (Int colIndexVal = colIndexLb; colIndexVal <= colIndexUb;
           ++colIndexVal) {
        ++ts;
        _solver->setValue(ts, rowIndex, rowIndexVal);
        _solver->setValue(ts, colIndex, colIndexVal);

        const Int expectedOutput =
            computeOutput(ts, rowIndex, colIndex, rowOffset, colOffset);

        invariant.notifyInputChanged(ts, LocalId(0));
        EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
      }
    }
  }
}

TEST_F(Element2dConstTest, NextInput) {
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = static_cast<Int>(numRows) - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = static_cast<Int>(numCols) - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    _solver->open();
    const VarViewId rowIndex =
        _solver->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarViewId colIndex =
        _solver->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<Int>>(matrix), rowOffset, colOffset);
    _solver->close();

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts), rowIndex);
      EXPECT_EQ(invariant.nextInput(ts), colIndex);
      EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    }
  }
}

TEST_F(Element2dConstTest, NotifyCurrentInputChanged) {
  Timestamp t0 = _solver->currentTimestamp() +
                 (numRows * numCols * static_cast<Int>(offsets.size())) + 1;
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = static_cast<Int>(numRows) - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::vector<Int> rowIndexValues(numRows, 0);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = static_cast<Int>(numCols) - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    _solver->open();
    const VarViewId rowIndex =
        _solver->makeIntVar(rowIndexValues.back(), rowIndexLb, rowIndexUb);

    const VarViewId colIndex =
        _solver->makeIntVar(colIndexValues.back(), colIndexLb, colIndexUb);

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<Int>>(matrix), rowOffset, colOffset);
    _solver->close();

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);
        const Timestamp ts = t0 + Timestamp(i * colIndexValues.size() + j);

        EXPECT_EQ(invariant.nextInput(ts), rowIndex);
        _solver->setValue(ts, rowIndex, rowIndexVal);
        invariant.notifyCurrentInputChanged(ts);

        EXPECT_EQ(invariant.nextInput(ts), colIndex);
        _solver->setValue(ts, colIndex, colIndexVal);
        invariant.notifyCurrentInputChanged(ts);

        EXPECT_EQ(_solver->value(ts, outputId),
                  computeOutput(ts, rowIndex, colIndex, rowOffset, colOffset));
      }
    }
  }
}

TEST_F(Element2dConstTest, Commit) {
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = static_cast<Int>(numRows) - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::vector<Int> rowIndexValues(numRows);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = static_cast<Int>(numCols) - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    _solver->open();
    const VarViewId rowIndex =
        _solver->makeIntVar(rowIndexValues.back(), rowIndexLb, rowIndexUb);

    const VarViewId colIndex =
        _solver->makeIntVar(colIndexValues.back(), colIndexLb, colIndexUb);

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<Int>>(matrix), rowOffset, colOffset);
    _solver->close();

    Int committedRowIndexValue = _solver->committedValue(rowIndex);
    Int committedColIndexValue = _solver->committedValue(colIndex);

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);

        const Timestamp ts = _solver->currentTimestamp() +
                             Timestamp(i * colIndexValues.size() + j);

        ASSERT_EQ(_solver->committedValue(rowIndex), committedRowIndexValue);
        ASSERT_EQ(_solver->committedValue(colIndex), committedColIndexValue);

        // Change row index
        _solver->setValue(ts, rowIndex, rowIndexVal);

        // notify row index change
        invariant.notifyInputChanged(ts, LocalId{0});
        invariant.notifyInputChanged(ts, LocalId{1});

        // incremental value from row index
        Int notifiedOutput = _solver->value(ts, outputId);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

        // Change col index
        _solver->setValue(ts, colIndex, colIndexVal);

        // notify col index change
        invariant.notifyInputChanged(ts, LocalId(0));

        // incremental value from col index
        notifiedOutput = _solver->value(ts, outputId);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

        _solver->commitIf(ts, VarId(rowIndex));
        committedRowIndexValue = _solver->value(ts, rowIndex);
        _solver->commitIf(ts, VarId(colIndex));
        committedColIndexValue = _solver->value(ts, colIndex);
        _solver->commitIf(ts, VarId(outputId));

        invariant.commit(ts);
        invariant.recompute(ts + 1);
        ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputId));
      }
    }
  }
}

class MockElement2dConst : public Element2dConst {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Element2dConst::registerVars();
  }
  explicit MockElement2dVar(SolverBase& solver, VarViewId output,
                            VarViewId index1, VarViewId index2,
                            std::vector<std::vector<Int>>&& matrix, Int offset1,
                            Int offset2)
      : Element2dConst(solver, output, index1, index2, std::move(matrix),
                       offset1, offset2) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Element2dConst::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Element2dConst::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Element2dConst::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Element2dConst::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Element2dConst::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(Element2dConstTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<std::vector<Int>> parMatrix(numRows, std::vector<Int>(numCols));
    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        parMatrix.at(i).at(j) = static_cast<Int>(i * numCols + j);
      }
    }
    VarViewId index1 = _solver->makeIntVar(1, 1, static_cast<Int>(numRows));
    VarViewId index2 = _solver->makeIntVar(1, 1, static_cast<Int>(numCols));
    VarViewId output = _solver->makeIntVar(-10, -100, 100);
    testNotifications<MockElement2dVar>(
        &_solver->makeInvariant<MockElement2dVar>(
            *_solver, output, index1, index2, std::move(parMatrix), 1, 1),
        {propMode, markingMode, 3, index1, 5, output});
  }
}

}  // namespace atlantis::testing
