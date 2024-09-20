#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/element2dVar.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class Element2dVarTest : public InvariantTest {
 protected:
  const size_t numRows = 4;
  const size_t numCols = 5;
  const Int inputLb = std::numeric_limits<Int>::min();
  const Int inputUb = std::numeric_limits<Int>::max();
  std::vector<std::pair<Int, Int>> offsets =
      cartesianProduct(std::vector<Int>{-10, 0, 10});
  std::vector<std::vector<VarViewId>> inputMatrix;
  std::uniform_int_distribution<Int> inputDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    inputMatrix.resize(numRows, std::vector<VarViewId>(numCols, NULL_ID));
    inputDist = std::uniform_int_distribution<Int>(inputLb, inputUb);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputMatrix.clear();
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

  VarViewId getInput(const Int rowIndexVal, const Int colIndexVal,
                     const Int rowOffset, const Int colOffset) {
    return inputMatrix.at(zeroBasedRowIndex(rowIndexVal, rowOffset))
        .at(zeroBasedColIndex(colIndexVal, colOffset));
  }

  Int computeOutput(const Timestamp ts, const VarViewId rowIndex,
                    const VarViewId colIndex, const Int rowOffset,
                    const Int colOffset) {
    return computeOutput(ts, _solver->value(ts, rowIndex),
                         _solver->value(ts, colIndex), rowOffset, colOffset);
  }

  Int computeOutput(const Timestamp ts, const Int rowIndexVal,
                    const Int colIndexVal, const Int rowOffset,
                    const Int colOffset) {
    return _solver->value(
        ts, getInput(rowIndexVal, colIndexVal, rowOffset, colOffset));
  }
};

TEST_F(Element2dVarTest, UpdateBounds) {
  EXPECT_TRUE(inputLb <= inputUb);
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

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            _solver->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarViewId outputId = _solver->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<VarViewId>>(inputMatrix), rowOffset, colOffset);
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
                minVal = std::min(minVal, _solver->lowerBound(
                                              getInput(rowIndexVal, colIndexVal,
                                                       rowOffset, colOffset)));
                maxVal = std::max(maxVal, _solver->upperBound(
                                              getInput(rowIndexVal, colIndexVal,
                                                       rowOffset, colOffset)));
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

TEST_F(Element2dVarTest, Recompute) {
  EXPECT_TRUE(inputLb <= inputUb);
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

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            _solver->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarViewId outputId = _solver->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<VarViewId>>(inputMatrix), rowOffset, colOffset);
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

TEST_F(Element2dVarTest, NotifyInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
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

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            _solver->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarViewId outputId = _solver->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<VarViewId>>(inputMatrix), rowOffset, colOffset);
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

        invariant.notifyInputChanged(ts, LocalId{0});
        EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
      }
    }
  }
}

TEST_F(Element2dVarTest, NextInput) {
  EXPECT_TRUE(inputLb <= inputUb);
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

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            _solver->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarViewId outputId = _solver->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<VarViewId>>(inputMatrix), rowOffset, colOffset);
    _solver->close();

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts), rowIndex);
      EXPECT_EQ(invariant.nextInput(ts), colIndex);
      EXPECT_EQ(invariant.nextInput(ts),
                getInput(_solver->value(ts, rowIndex),
                         _solver->value(ts, colIndex), rowOffset, colOffset));
      EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    }
  }
}

TEST_F(Element2dVarTest, NotifyCurrentInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
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

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            _solver->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarViewId outputId = _solver->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<VarViewId>>(inputMatrix), rowOffset, colOffset);
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

        const VarViewId curInput = invariant.nextInput(ts);
        EXPECT_EQ(curInput,
                  getInput(rowIndexVal, colIndexVal, rowOffset, colOffset));

        const Int oldInputVal = _solver->value(ts, curInput);
        do {
          _solver->setValue(ts, curInput, inputDist(gen));
        } while (_solver->value(ts, curInput) == oldInputVal);

        invariant.notifyCurrentInputChanged(ts);
        EXPECT_EQ(_solver->value(ts, outputId),
                  computeOutput(ts, rowIndex, colIndex, rowOffset, colOffset));
      }
    }
  }
}

TEST_F(Element2dVarTest, Commit) {
  EXPECT_TRUE(inputLb <= inputUb);
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

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            _solver->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarViewId outputId = _solver->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputId, rowIndex, colIndex,
        std::vector<std::vector<VarViewId>>(inputMatrix), rowOffset, colOffset);
    _solver->close();

    Int committedRowIndexValue = _solver->committedValue(rowIndex);
    Int committedColIndexValue = _solver->committedValue(colIndex);

    std::vector<std::vector<Int>> committedInputValues(
        numRows, std::vector<Int>(numCols));

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        committedInputValues.at(i).at(j) =
            _solver->committedValue(inputMatrix.at(i).at(j));
      }
    }

    Timestamp ts = _solver->currentTimestamp();
    for (const Int rowIndexVal : rowIndexValues) {
      for (const Int colIndexVal : colIndexValues) {
        ++ts;

        ASSERT_EQ(_solver->committedValue(rowIndex), committedRowIndexValue);
        ASSERT_EQ(_solver->committedValue(colIndex), committedColIndexValue);

        for (size_t i = 0; i < numRows; ++i) {
          for (size_t j = 0; j < numCols; ++j) {
            ASSERT_EQ(_solver->committedValue(inputMatrix.at(i).at(j)),
                      committedInputValues.at(i).at(j));
          }
        }

        // Change row index
        _solver->setValue(ts, rowIndex, rowIndexVal);

        // notify row index change
        invariant.notifyInputChanged(ts, LocalId{numRows * numCols});

        // incremental value from row index
        Int notifiedOutput = _solver->value(ts, outputId);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

        // Change col index
        _solver->setValue(ts, colIndex, colIndexVal);

        // notify col index change
        invariant.notifyInputChanged(ts, LocalId{numRows * numCols + 1});

        // incremental value from col index
        notifiedOutput = _solver->value(ts, outputId);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

        // Change input
        const VarViewId curInput =
            getInput(rowIndexVal, colIndexVal, rowOffset, colOffset);
        const Int oldInputVal = _solver->value(ts, curInput);
        do {
          _solver->setValue(ts, curInput, inputDist(gen));
        } while (_solver->value(ts, curInput) == oldInputVal);

        // notify input change
        invariant.notifyInputChanged(
            ts, LocalId{zeroBasedRowIndex(rowIndexVal, rowOffset) * numCols +
                        zeroBasedColIndex(colIndexVal, colOffset)});

        // incremental value from input
        notifiedOutput = _solver->value(ts, outputId);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

        _solver->commitIf(ts, VarId(rowIndex));
        committedRowIndexValue = _solver->value(ts, rowIndex);
        _solver->commitIf(ts, VarId(colIndex));
        committedColIndexValue = _solver->value(ts, colIndex);
        _solver->commitIf(ts, VarId(curInput));
        committedInputValues.at(zeroBasedRowIndex(rowIndexVal, rowOffset))
            .at(zeroBasedColIndex(colIndexVal, colOffset)) =
            _solver->value(ts, curInput);
        _solver->commitIf(ts, VarId(outputId));

        invariant.commit(ts);
        invariant.recompute(ts + 1);
        ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputId));
      }
    }
  }
}

TEST_F(Element2dVarTest, Prop) {
  _solver->open();
  const size_t numRows = 2;
  const size_t numCols = 2;
  std::vector<std::vector<VarViewId>> varMatrix(
      numRows, std::vector<VarViewId>(numCols, NULL_ID));

  for (size_t i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      varMatrix.at(i).at(j) = _solver->makeIntVar(0, 0, 5);
    }
  }

  const VarViewId index1 = _solver->makeIntVar(1, 1, 2);
  const VarViewId index2 = _solver->makeIntVar(1, 1, 2);

  const Int offset1 = 1;
  const Int offset2 = 1;

  const VarViewId output = _solver->makeIntVar(0, 0, 100);

  auto& inv = _solver->makeInvariant<Element2dVar>(
      *_solver, output, index1, index2,
      std::vector<std::vector<VarViewId>>(varMatrix), offset1, offset2);
  _solver->close();

  std::vector<VarViewId> inputVars;
  inputVars.reserve(numRows * numCols + 2);
  for (size_t i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      inputVars.push_back(varMatrix.at(i).at(j));
    }
  }
  inputVars.push_back(index1);
  inputVars.push_back(index2);

  std::vector<Int> inputVals = createInputVals(inputVars);

  while (increaseNextVal(inputVars, inputVals)) {
    _solver->beginMove();
    setVarVals(inputVars, inputVals);
    _solver->endMove();

    _solver->beginCommit();
    _solver->query(output);
    _solver->endCommit();

    const Int actual = _solver->committedValue(output);
    const Int index1Val = inputVals.at(inputVals.size() - 2) - offset1;
    const Int index2Val = inputVals.back() - offset2;
    RC_ASSERT(inv.dynamicInputVar(_solver->currentTimestamp()) ==
              varMatrix.at(index1Val).at(index2Val));
    const Int expected = inputVals.at(index1Val * numCols + index2Val);
    if (expected != actual) {
      RC_ASSERT(expected == actual);
    }
  }
}

RC_GTEST_FIXTURE_PROP(Element2dVarTest, rapidcheck, ()) {
  _solver->open();

  const Int lb = -100;
  const Int ub = 100;

  const Int numRows = *rc::gen::inRange<Int>(2, 5);
  const Int numCols = *rc::gen::inRange<Int>(2, 5);

  const Int rowOffset = *rc::gen::inRange<Int>(-10, 10);
  const Int colOffset = *rc::gen::inRange<Int>(-10, 10);

  std::vector<std::vector<VarViewId>> varMatrix(numRows,
                                                std::vector<VarViewId>());

  for (Int r = 0; r < numRows; ++r) {
    varMatrix.at(r).reserve(numCols);
    for (Int c = 0; c < numCols; ++c) {
      varMatrix.at(r).emplace_back(
          _solver->makeIntVar(r * numRows + c, lb, ub));
    }
  }

  const Int rowUb = numRows + rowOffset - 1;
  const Int colUb = numCols + colOffset - 1;

  VarViewId rowIndex = _solver->makeIntVar(rowOffset, rowOffset, rowUb);
  VarViewId colIndex = _solver->makeIntVar(colOffset, colOffset, colUb);

  VarViewId output = _solver->makeIntVar(0, 0, 0);

  _solver->makeInvariant<Element2dVar>(
      *_solver, output, rowIndex, colIndex,
      std::vector<std::vector<VarViewId>>(varMatrix), rowOffset, colOffset);

  _solver->close();

  std::vector<VarViewId> modifiedVars{rowIndex, colIndex, VarViewId(NULL_ID),
                                      VarViewId(NULL_ID)};
  const size_t numMoves = 3;
  const size_t numProbes = 10;

  for (size_t m = numMoves; m > 0; --m) {
    const Int committedRow = _solver->committedValue(rowIndex);
    RC_ASSERT(committedRow >= rowOffset);
    RC_ASSERT(committedRow < numRows + rowOffset);
    const Int committedCol = _solver->committedValue(colIndex);
    RC_ASSERT(committedCol >= colOffset);
    RC_ASSERT(committedCol < numCols + colOffset);

    modifiedVars.at(2) =
        varMatrix.at(committedRow - rowOffset).at(committedCol - colOffset);

    for (size_t p = numProbes; p > 0; --p) {
      RC_ASSERT(committedRow == _solver->committedValue(rowIndex));
      RC_ASSERT(committedCol == _solver->committedValue(colIndex));

      RC_ASSERT(_solver->committedValue(output) ==
                _solver->committedValue(modifiedVars.at(2)));

      _solver->beginMove();

      const bool modifyRow = *rc::gen::arbitrary<bool>();
      const Int newRow = modifyRow
                             ? *rc::gen::inRange(rowOffset, numRows + rowOffset)
                             : committedRow;
      if (modifyRow) {
        _solver->setValue(rowIndex, newRow);
      }

      const bool modifyCol = *rc::gen::arbitrary<bool>();
      const Int newCol = modifyCol
                             ? *rc::gen::inRange(colOffset, numCols + colOffset)
                             : committedCol;
      if (modifyCol) {
        _solver->setValue(colIndex, newCol);
      }

      const bool modifyCommitted = *rc::gen::arbitrary<bool>();
      if (modifyCommitted) {
        const Int newCommittedVal = *rc::gen::inRange(lb, ub + 1);
        _solver->setValue(modifiedVars.at(2), newCommittedVal);
      }

      modifiedVars.at(3) =
          varMatrix.at(newRow - rowOffset).at(newCol - colOffset);

      const bool modifyNew = *rc::gen::arbitrary<bool>();
      if (modifyNew) {
        const Int newCurVal = *rc::gen::inRange(lb, ub + 1);
        _solver->setValue(modifiedVars.at(2), newCurVal);
      }

      _solver->endMove();

      if (p != 1) {
        _solver->beginProbe();
      } else {
        _solver->beginCommit();
      }
      _solver->query(output);
      if (p != 1) {
        _solver->endProbe();
      } else {
        _solver->endCommit();
      }
      RC_ASSERT(_solver->currentValue(modifiedVars.at(3)) ==
                _solver->currentValue(output));
    }
    for (const VarViewId& id : modifiedVars) {
      RC_ASSERT(_solver->currentValue(id) == _solver->committedValue(id));
    }
  }
}

class MockElement2dVar : public Element2dVar {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Element2dVar::registerVars();
  }
  explicit MockElement2dVar(SolverBase& solver, VarViewId output,
                            VarViewId index1, VarViewId index2,
                            std::vector<std::vector<VarViewId>>&& varMatrix,
                            Int offset1, Int offset2)
      : Element2dVar(solver, output, index1, index2, std::move(varMatrix),
                     offset1, offset2) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Element2dVar::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Element2dVar::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Element2dVar::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Element2dVar::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Element2dVar::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(Element2dVarTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<std::vector<VarViewId>> varMatrix(
        numRows, std::vector<VarViewId>(numCols, NULL_ID));
    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        varMatrix.at(i).at(j) =
            _solver->makeIntVar(static_cast<Int>(i * numCols + j), -100, 100);
      }
    }
    VarViewId index1 = _solver->makeIntVar(1, 1, static_cast<Int>(numRows));
    VarViewId index2 = _solver->makeIntVar(1, 1, static_cast<Int>(numCols));
    VarViewId output = _solver->makeIntVar(-10, -100, 100);
    MockElement2dVar invariant =
        MockElement2dVar(*_solver, output, index1, index2,
                         std::vector<std::vector<VarViewId>>(varMatrix), 1, 1);

    testNotifications<MockElement2dVar>(
        &_solver->makeInvariant<MockElement2dVar>(
            *_solver, output, index1, index2, std::move(varMatrix), 1, 1),
        {propMode, markingMode, 4, index1, 5, output});
  }
}

}  // namespace atlantis::testing
