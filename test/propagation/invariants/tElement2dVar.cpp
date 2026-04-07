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

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    dynamicVarDist = std::uniform_int_distribution<Int>(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  }

  Element2dVar& generate() {
    varMatrix.resize(numRows, std::vector<VarViewId>(numCols, NULL_ID));
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
    return _solver->makeInvariant<Element2dVar>(
        *_solver, outputVar, rowIndexVar, colIndexVar,
        std::vector<std::vector<VarViewId>>(varMatrix), rowOffset, colOffset);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    varMatrix.clear();
  }

  [[nodiscard]] size_t zeroBasedRowIndex(Int rowIndexVal) const {
    EXPECT_LE(rowOffset, rowIndexVal);
    EXPECT_LT(rowIndexVal - rowOffset, numRows);
    return rowIndexVal - rowOffset;
  }

  [[nodiscard]] size_t zeroBasedColIndex(Int colIndexVal) const {
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

    _solver->open();
    auto& invariant = generate();
    _solver->close();

    for (Int minRowIndex = rowIndexLb(); minRowIndex <= rowIndexUb();
         ++minRowIndex) {
      for (Int maxRowIndex = rowIndexUb(); maxRowIndex >= minRowIndex;
           --maxRowIndex) {
        _solver->updateBounds(VarId(rowIndexVar), minRowIndex, maxRowIndex,
                              false);
        for (Int minColIndex = colIndexLb(); minColIndex <= colIndexUb();
             ++minColIndex) {
          for (Int maxColIndex = colIndexUb(); maxColIndex >= minColIndex;
               --maxColIndex) {
            _solver->updateBounds(VarId(colIndexVar), minColIndex, maxColIndex,
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

    _solver->open();
    auto& invariant = generate();
    _solver->close();

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

    _solver->open();
    auto& invariant = generate();
    _solver->close();

    Timestamp ts = _solver->currentTimestamp();

    const Int i{-1};

    for (Int rowIndexVal = rowIndexLb(); rowIndexVal <= rowIndexUb();
         ++rowIndexVal) {
      for (Int colIndexVal = colIndexLb(); colIndexVal <= colIndexUb();
           ++colIndexVal) {
        ++ts;

        _solver->setValue(ts, rowIndexVar, rowIndexVal);
        _solver->setValue(ts, colIndexVar, colIndexVal);

        const Int expectedOutput = computeOutput(ts);

        invariant.notifyInputChanged(ts, LocalId(i));
        EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
      }
    }
  }
}

TEST_F(Element2dVarTest, NextInput) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    _solver->open();
    auto& invariant = generate();
    _solver->close();

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

    _solver->open();
    auto& invariant = generate();
    _solver->close();

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);
        const Timestamp ts = t0 + Timestamp(i * colIndexValues.size() + j);

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

    _solver->open();
    auto& invariant = generate();
    _solver->close();

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
        invariant.notifyInputChanged(ts, LocalId(0));

        // incremental value from row index
        Int notifiedOutput = _solver->value(ts, outputVar);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

        // Change col index
        _solver->setValue(ts, colIndexVar, colIndexVal);

        // notify col index change
        invariant.notifyInputChanged(ts, LocalId(0));

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
        invariant.notifyInputChanged(ts, LocalId(0));

        // incremental value from input
        notifiedOutput = _solver->value(ts, outputVar);
        invariant.recompute(ts);

        ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

        _solver->commitIf(ts, VarId(rowIndexVar));
        committedRowIndexValue = _solver->value(ts, rowIndexVar);
        _solver->commitIf(ts, VarId(colIndexVar));
        committedColIndexValue = _solver->value(ts, colIndexVar);
        _solver->commitIf(ts, VarId(curInput));
        committedInputValues.at(zeroBasedRowIndex(rowIndexVal))
            .at(zeroBasedColIndex(colIndexVal)) = _solver->value(ts, curInput);
        _solver->commitIf(ts, VarId(outputVar));

        invariant.commit(ts);
        invariant.recompute(ts + 1);
        ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
      }
    }
  }
}

RC_GTEST_FIXTURE_PROP(Element2dVarTest, rapidcheck, ()) {
  numRows = *rc::gen::inRange<Int>(1, 100);
  numCols = *rc::gen::inRange<Int>(1, 100);

  rowOffset = *rc::gen::inRange(std::numeric_limits<Int>::min() + 200,
                                std::numeric_limits<Int>::max() - 200);
  colOffset = *rc::gen::inRange(std::numeric_limits<Int>::min() + 200,
                                std::numeric_limits<Int>::max() - 200);

  std::vector<VarViewId> dynamicInputVars;
  dynamicInputVars.reserve(numRows * numCols);

  _solver->open();
  generate();
  _solver->close();

  for (const auto& row : varMatrix) {
    for (const VarViewId& dynId : row) {
      dynamicInputVars.emplace_back(dynId);
    }
  }

  const std::vector<VarViewId> staticInputVars{rowIndexVar, colIndexVar};

  std::vector<std::uniform_int_distribution<Int>> indexDists{rowIndexDist,
                                                             colIndexDist};

  constexpr size_t numCommits = 3;
  constexpr size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (const auto& dVar : dynamicInputVars) {
        if (randBool()) {
          _solver->setValue(dVar, dynamicVarDist(gen));
        }
      }
      for (size_t i = 0; i < staticInputVars.size(); ++i) {
        if (randBool()) {
          _solver->setValue(staticInputVars.at(i), indexDists.at(i)(gen));
        }
      }
      _solver->endMove();

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      _solver->query(outputVar);
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(_solver->currentValue(outputVar) == computeOutput());
    }
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
  }
}

TEST_F(Element2dVarTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    _solver = std::make_shared<Solver>();
    _solver->open();
    _solver->setPropagationMode(propMode);
    _solver->setOutputToInputMarkingMode(markingMode);

    std::vector<std::vector<VarViewId>> varMatrix(
        numRows, std::vector<VarViewId>(numCols, NULL_ID));
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
