#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/element2dVar.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class Element2dVarTest : public InvariantTest {
 protected:
  Int numRows{4};
  Int numCols{5};
  Int rowOffset{1};
  Int colOffset{1};

  std::vector<std::pair<Int, Int>> offsets =
      cartesianProduct(std::vector<Int>{-10, 0, 10});

  std::uniform_int_distribution<Int> dynamicVarDist;
  std::uniform_int_distribution<Int> rowIndexDist;
  std::uniform_int_distribution<Int> colIndexDist;

  std::vector<std::vector<VarViewId>> varMatrix;
  VarViewId rowIndexVar{NULL_ID};
  VarViewId colIndexVar{NULL_ID};
  VarViewId outputVar{NULL_ID};

  [[nodiscard]] Int rowIndexLb() const { return rowOffset; }
  [[nodiscard]] Int rowIndexUb() const { return rowOffset + numRows - 1; }
  [[nodiscard]] Int colIndexLb() const { return colOffset; }
  [[nodiscard]] Int colIndexUb() const { return colOffset + numCols - 1; }

  void SetUp() override {
    InvariantTest::SetUp();
    dynamicVarDist = std::uniform_int_distribution<Int>(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  }

  Element2dVar& generate() {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    for (const auto varId : _solver->searchVars()) {
      _solver->setValue(_solver->currentTimestamp(), varId, _solver->committedValue(varId));
    }

    varMatrix.resize(numRows,
                     std::vector<VarViewId>(numCols, VAR_VIEW_NULL_ID));
    for (Int r = 0; r < numRows; ++r) {
      for (Int c = 0; c < numCols; ++c) {
        varMatrix.at(r).at(c) =
            makeIntVar(std::numeric_limits<Int>::min(),
                       std::numeric_limits<Int>::max(), dynamicVarDist);
      }
    }

    rowIndexDist =
        std::uniform_int_distribution<Int>(rowIndexLb(), rowIndexUb());
    colIndexDist =
        std::uniform_int_distribution<Int>(colIndexLb(), colIndexUb());

    rowIndexVar = makeIntVar(rowIndexLb(), rowIndexUb(), rowIndexDist);
    colIndexVar = makeIntVar(colIndexLb(), colIndexUb(), colIndexDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    Element2dVar& invariant = _solver->makeInvariant<Element2dVar>(
        *_solver, outputVar, rowIndexVar, colIndexVar,
        std::vector<std::vector<VarViewId>>(varMatrix), rowOffset, colOffset);
    _solver->close();
    return invariant;
  }

  void TearDown() override {
    InvariantTest::TearDown();
    varMatrix.clear();
  }

  [[nodiscard]] size_t zeroBasedRowIndex(const Int rowIndexVal) const {
    EXPECT_LE(rowOffset, rowIndexVal);
    EXPECT_LT(rowIndexVal - rowOffset, numRows);
    return rowIndexVal - rowOffset;
  }

  [[nodiscard]] size_t zeroBasedColIndex(const Int colIndexVal) const {
    EXPECT_LE(colOffset, colIndexVal);
    EXPECT_LT(colIndexVal - colOffset, numCols);
    return colIndexVal - colOffset;
  }

  [[nodiscard]] VarViewId getInput(Int rowIndexVal, Int colIndexVal) const {
    return varMatrix.at(zeroBasedRowIndex(rowIndexVal))
        .at(zeroBasedColIndex(colIndexVal));
  }

  [[nodiscard]] Int computeOutput(Timestamp ts) const {
    return computeOutput(ts, _solver->value(ts, rowIndexVar),
                         _solver->value(ts, colIndexVar));
  }

  [[nodiscard]] Int computeOutput(bool committedValue = false) const {
    return computeOutput(committedValue ? _solver->committedValue(rowIndexVar)
                                        : _solver->currentValue(rowIndexVar),
                         committedValue ? _solver->committedValue(colIndexVar)
                                        : _solver->currentValue(colIndexVar),
                         committedValue);
  }

  [[nodiscard]] Int computeOutput(Timestamp ts, Int rowIndexVal,
                                  Int colIndexVal) const {
    return _solver->value(ts, getInput(rowIndexVal, colIndexVal));
  }

  [[nodiscard]] Int computeOutput(Int rowIndexVal, Int colIndexVal,
                                  bool committedValue = false) const {
    return committedValue
               ? _solver->committedValue(getInput(rowIndexVal, colIndexVal))
               : _solver->currentValue(getInput(rowIndexVal, colIndexVal));
  }
};

TEST_F(Element2dVarTest, UpdateBounds) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    for (Int minRowIndex = rowIndexLb(); minRowIndex <= rowIndexUb();
         ++minRowIndex) {
      for (Int maxRowIndex = rowIndexUb(); maxRowIndex >= minRowIndex;
           --maxRowIndex) {
        _solver->updateBounds(VarId{rowIndexVar}, minRowIndex, maxRowIndex,
                              false);
        for (Int minColIndex = colIndexLb(); minColIndex <= colIndexUb();
             ++minColIndex) {
          for (Int maxColIndex = colIndexUb(); maxColIndex >= minColIndex;
               --maxColIndex) {
            _solver->updateBounds(VarId{colIndexVar}, minColIndex, maxColIndex,
                                  false);
            invariant.updateBounds(false);
            Int minVal = std::numeric_limits<Int>::max();
            Int maxVal = std::numeric_limits<Int>::min();
            for (Int rowIndexVal = minRowIndex; rowIndexVal <= maxRowIndex;
                 ++rowIndexVal) {
              for (Int colIndexVal = minColIndex; colIndexVal <= maxColIndex;
                   ++colIndexVal) {
                minVal = std::min(minVal, _solver->lowerBound(getInput(
                                              rowIndexVal, colIndexVal)));
                maxVal = std::max(maxVal, _solver->upperBound(getInput(
                                              rowIndexVal, colIndexVal)));
              }
            }
            EXPECT_EQ(minVal, _solver->lowerBound(outputVar));
            EXPECT_EQ(maxVal, _solver->upperBound(outputVar));
          }
        }
      }
    }
  }
}

TEST_F(Element2dVarTest, Recompute) {
  generateState = GenerateState::LB;

  Timestamp ts =
      _solver->currentTimestamp() + static_cast<Int>(offsets.size()) + 2;

  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    for (Int rowIndexVal = rowIndexLb(); rowIndexVal <= rowIndexUb();
         ++rowIndexVal) {
      ++ts;
      _solver->setValue(ts, rowIndexVar, rowIndexVal);
      EXPECT_EQ(_solver->value(ts, rowIndexVar), rowIndexVal);
      for (Int colIndexVal = colIndexLb(); colIndexVal <= colIndexUb();
           ++colIndexVal) {
        _solver->setValue(ts, colIndexVar, colIndexVal);
        EXPECT_EQ(_solver->value(ts, colIndexVar), colIndexVal);

        const Int expectedOutput = computeOutput(ts);
        invariant.recompute(ts);
        EXPECT_EQ(_solver->value(ts, rowIndexVar), rowIndexVal);

        EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
      }
    }
  }
}

TEST_F(Element2dVarTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    Timestamp ts = _solver->currentTimestamp();

    for (Int rowIndexVal = rowIndexLb(); rowIndexVal <= rowIndexUb();
         ++rowIndexVal) {
      for (Int colIndexVal = colIndexLb(); colIndexVal <= colIndexUb();
           ++colIndexVal) {
        constexpr Int i{-1};
        ++ts;

        _solver->setValue(ts, rowIndexVar, rowIndexVal);
        _solver->setValue(ts, colIndexVar, colIndexVal);

        const Int expectedOutput = computeOutput(ts);

        invariant.notifyInputChanged(ts, i);
        EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
      }
    }
  }
}

TEST_F(Element2dVarTest, NextInput) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts), rowIndexVar);
      EXPECT_EQ(invariant.nextInput(ts), colIndexVar);
      EXPECT_EQ(invariant.nextInput(ts),
                getInput(_solver->value(ts, rowIndexVar),
                         _solver->value(ts, colIndexVar)));
      EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    }
  }
}

TEST_F(Element2dVarTest, NotifyCurrentInputChanged) {
  const Timestamp t0 = _solver->currentTimestamp() +
                       (numRows * numCols * static_cast<Int>(offsets.size())) +
                       1;
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    std::vector<Int> rowIndexValues(numRows, 0);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::ranges::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::ranges::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    auto& invariant = generate();

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);
        const Timestamp ts = t0 + i * colIndexValues.size() + j;

        EXPECT_EQ(invariant.nextInput(ts), rowIndexVar);
        _solver->setValue(ts, rowIndexVar, rowIndexVal);
        invariant.notifyCurrentInputChanged(ts);

        EXPECT_EQ(invariant.nextInput(ts), colIndexVar);
        _solver->setValue(ts, colIndexVar, colIndexVal);
        invariant.notifyCurrentInputChanged(ts);

        EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));

        const VarViewId curInput = invariant.nextInput(ts);
        EXPECT_EQ(curInput, getInput(rowIndexVal, colIndexVal));

        const Int oldInputVal = _solver->value(ts, curInput);
        do {
          _solver->setValue(ts, curInput, dynamicVarDist(gen));
        } while (_solver->value(ts, curInput) == oldInputVal);

        invariant.notifyCurrentInputChanged(ts);
        EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
      }
    }
  }
}

TEST_F(Element2dVarTest, Commit) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    std::vector<Int> rowIndexValues(numRows);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::ranges::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::ranges::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    auto& invariant = generate();

    Int committedRowIndexValue = _solver->committedValue(rowIndexVar);
    Int committedColIndexValue = _solver->committedValue(colIndexVar);

    std::vector<std::vector<Int>> committedInputValues(
        numRows, std::vector<Int>(numCols));

    for (Int i = 0; i < numRows; ++i) {
      for (Int j = 0; j < numCols; ++j) {
        committedInputValues.at(i).at(j) =
            _solver->committedValue(varMatrix.at(i).at(j));
      }
    }

    Timestamp ts = _solver->currentTimestamp();
    for (const Int rowIndexVal : rowIndexValues) {
      for (const Int colIndexVal : colIndexValues) {
        ++ts;

        ASSERT_EQ(_solver->committedValue(rowIndexVar), committedRowIndexValue);
        ASSERT_EQ(_solver->committedValue(colIndexVar), committedColIndexValue);

        for (Int i = 0; i < numRows; ++i) {
          for (Int j = 0; j < numCols; ++j) {
            ASSERT_EQ(_solver->committedValue(varMatrix.at(i).at(j)),
                      committedInputValues.at(i).at(j));
          }
        }

        // Change row index
        _solver->setValue(ts, rowIndexVar, rowIndexVal);

        // notify row index change
        invariant.notifyInputChanged(ts, numRows * numCols);

        // incremental value from row index
        Int notifiedOutput = _solver->value(ts, outputVar);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

        // Change col index
        _solver->setValue(ts, colIndexVar, colIndexVal);

        // notify col index change
        invariant.notifyInputChanged(ts, numRows * numCols + 1);

        // incremental value from col index
        notifiedOutput = _solver->value(ts, outputVar);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

        // Change input
        const VarViewId curInput = getInput(rowIndexVal, colIndexVal);
        const Int oldInputVal = _solver->value(ts, curInput);
        do {
          _solver->setValue(ts, curInput, dynamicVarDist(gen));
        } while (_solver->value(ts, curInput) == oldInputVal);

        // notify input change
        invariant.notifyInputChanged(
            ts, LocalId{zeroBasedRowIndex(rowIndexVal) * numCols +
                        zeroBasedColIndex(colIndexVal)});

        // incremental value from input
        notifiedOutput = _solver->value(ts, outputVar);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

        _solver->commitIf(ts, VarId{rowIndexVar});
        committedRowIndexValue = _solver->value(ts, rowIndexVar);
        _solver->commitIf(ts, VarId{colIndexVar});
        committedColIndexValue = _solver->value(ts, colIndexVar);
        _solver->commitIf(ts, VarId{curInput});
        committedInputValues.at(zeroBasedRowIndex(rowIndexVal))
            .at(zeroBasedColIndex(colIndexVal)) = _solver->value(ts, curInput);
        _solver->commitIf(ts, VarId{outputVar});

        invariant.commit(ts);
        invariant.recompute(ts + 1);
        ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
      }
    }
  }
}

TEST_F(Element2dVarTest, Prop) {
  _solver->open();
  const size_t numRows = 2;
  const size_t numCols = 2;
  std::vector<std::vector<VarViewId>> varMatrix(
      numRows, std::vector<VarViewId>(numCols, VAR_VIEW_NULL_ID));

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

TEST_F(Element2dVarTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    _solver = std::make_shared<Solver>();
    _solver->open();
    _solver->setPropagationMode(propMode);
    _solver->setOutputToInputMarkingMode(markingMode);

    std::vector<std::vector<VarViewId>> varMatrix(
        numRows, std::vector<VarViewId>(numCols, VAR_VIEW_NULL_ID));
    for (Int i = 0; i < numRows; ++i) {
      for (Int j = 0; j < numCols; ++j) {
        varMatrix.at(i).at(j) = _solver->makeIntVar(i * numCols + j, -100, 100);
      }
    }
    VarViewId index1 = _solver->makeIntVar(1, 1, numRows);
    VarViewId index2 = _solver->makeIntVar(1, 1, numCols);
    VarViewId output = _solver->makeIntVar(-10, -100, 100);
    const VarViewId selectedInput = varMatrix.at(1).at(2);
    _solver->makeInvariant<Element2dVar>(*_solver, output, index1, index2,
                                         std::move(varMatrix), 1, 1);
    _solver->close();

    _solver->beginMove();
    _solver->setValue(index1, 2);
    _solver->setValue(index2, 3);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(output);
    EXPECT_LE(_solver->lowerBound(output), _solver->currentValue(output));
    EXPECT_GE(_solver->upperBound(output), _solver->currentValue(output));
    _solver->endProbe();

    _solver->beginMove();
    _solver->setValue(selectedInput, 42);
    _solver->endMove();

    _solver->beginProbe();
    _solver->query(output);
    EXPECT_LE(_solver->lowerBound(output), _solver->currentValue(output));
    EXPECT_GE(_solver->upperBound(output), _solver->currentValue(output));
    _solver->endProbe();
  }
}

}  // namespace atlantis::testing
