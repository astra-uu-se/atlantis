#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class Element2dConstTest : public InvariantTest {
 protected:
  Int numRows{4};
  Int numCols{5};
  Int rowOffset{1};
  Int colOffset{1};

  std::vector<std::pair<Int, Int>> offsets =
      cartesianProduct(std::vector<Int>{-10, 0, 10});

  std::vector<std::vector<Int>> parMatrix;

  VarViewId rowIndexVar{NULL_ID};
  VarViewId colIndexVar{NULL_ID};
  VarViewId outputVar{NULL_ID};
  std::vector<VarViewId> inputVars;

  std::uniform_int_distribution<Int> parDist;
  std::uniform_int_distribution<Int> rowIndexVarDist;
  std::uniform_int_distribution<Int> colIndexVarDist;

  Int rowIndexLb() const { return rowOffset; }
  Int rowIndexUb() const { return rowOffset + numRows - 1; }
  Int colIndexLb() const { return colOffset; }
  Int colIndexUb() const { return colOffset + numCols - 1; }

 public:
  void SetUp() override {
    InvariantTest::SetUp();

    parDist = std::uniform_int_distribution<Int>(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  }

  Element2dConst& generate() {
    parMatrix.resize(numRows, std::vector<Int>(numCols));
    for (size_t i = 0; i < static_cast<size_t>(numRows); ++i) {
      for (size_t j = 0; j < static_cast<size_t>(numCols); ++j) {
        parMatrix.at(i).at(j) = parDist(gen);
      }
    }

    rowIndexVarDist =
        std::uniform_int_distribution<Int>(rowIndexLb(), rowIndexUb());
    colIndexVarDist =
        std::uniform_int_distribution<Int>(colIndexLb(), colIndexUb());

    if (!_solver->isOpen()) {
      _solver->open();
    }

    rowIndexVar = makeIntVar(rowIndexLb(), rowIndexUb(), rowIndexVarDist);
    colIndexVar = makeIntVar(colIndexLb(), colIndexUb(), colIndexVarDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    Element2dConst& invariant = _solver->makeInvariant<Element2dConst>(
        *_solver, outputVar, rowIndexVar, colIndexVar,
        std::vector<std::vector<Int>>(parMatrix), rowOffset, colOffset);
    _solver->close();
    return invariant;
  }

  void TearDown() override {
    InvariantTest::TearDown();
    parMatrix.clear();
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

  Int computeOutput(Timestamp ts) {
    return computeOutput(_solver->value(ts, rowIndexVar),
                         _solver->value(ts, colIndexVar));
  }

  Int computeOutput(Int rowIndexVal, Int colIndexVal) {
    return parMatrix.at(zeroBasedRowIndex(rowIndexVal))
        .at(zeroBasedColIndex(colIndexVal));
  }

  Int computeOutput(bool committedValue = false) {
    return parMatrix
        .at(zeroBasedRowIndex(committedValue
                                  ? _solver->committedValue(rowIndexVar)
                                  : _solver->currentValue(rowIndexVar)))
        .at(zeroBasedColIndex(committedValue
                                  ? _solver->committedValue(colIndexVar)
                                  : _solver->currentValue(colIndexVar)));
  }
};

TEST_F(Element2dConstTest, UpdateBounds) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();
    _solver->open();

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
                minVal =
                    std::min(minVal, computeOutput(rowIndexVal, colIndexVal));
                maxVal =
                    std::max(maxVal, computeOutput(rowIndexVal, colIndexVal));
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

TEST_F(Element2dConstTest, Recompute) {
  generateState = GenerateState::LB;

  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    std::vector<VarViewId> inputVars{rowIndexVar, colIndexVar};

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const Int expectedOutput = computeOutput(ts);
      invariant.recompute(ts);
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(Element2dConstTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    std::vector<VarViewId> inputVars{rowIndexVar, colIndexVar};

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const Int expectedOutput = computeOutput(ts);
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(Element2dConstTest, NextInput) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    auto& invariant = generate();

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts), rowIndexVar);
      EXPECT_EQ(invariant.nextInput(ts), colIndexVar);
      EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    }
  }
}

TEST_F(Element2dConstTest, NotifyCurrentInputChanged) {
  Timestamp t0 = _solver->currentTimestamp() +
                 (numRows * numCols * static_cast<Int>(offsets.size())) + 1;
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    std::vector<Int> rowIndexValues(numRows, 0);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    auto& invariant = generate();

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
      }
    }
  }
}

TEST_F(Element2dConstTest, Commit) {
  for (const auto& [ro, co] : offsets) {
    rowOffset = ro;
    colOffset = co;

    std::vector<Int> rowIndexValues(numRows);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::uniform_int_distribution<Int> rowIndexVarDist(rowIndexLb(),
                                                       rowIndexUb());

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    std::uniform_int_distribution<Int> colIndexVarDist(colIndexLb(),
                                                       colIndexUb());

    auto& invariant = generate();

    Int committedRowIndexValue = _solver->committedValue(rowIndexVar);
    Int committedColIndexValue = _solver->committedValue(colIndexVar);

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);

        const Timestamp ts = _solver->currentTimestamp() +
                             Timestamp(i * colIndexValues.size() + j);

        ASSERT_EQ(_solver->committedValue(rowIndexVar), committedRowIndexValue);
        ASSERT_EQ(_solver->committedValue(colIndexVar), committedColIndexValue);

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

        _solver->commitIf(ts, VarId(rowIndexVar));
        committedRowIndexValue = _solver->value(ts, rowIndexVar);
        _solver->commitIf(ts, VarId(colIndexVar));
        committedColIndexValue = _solver->value(ts, colIndexVar);
        _solver->commitIf(ts, VarId(outputVar));

        invariant.commit(ts);
        invariant.recompute(ts + 1);
        ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
      }
    }
  }
}

RC_GTEST_FIXTURE_PROP(Element2dConstTest, rapidcheck, ()) {
  numRows = *rc::gen::inRange<Int>(1, 100);
  numCols = *rc::gen::inRange<Int>(1, 100);

  rowOffset = *rc::gen::inRange(std::numeric_limits<Int>::min() + 200,
                                std::numeric_limits<Int>::max() - 200);
  colOffset = *rc::gen::inRange(std::numeric_limits<Int>::min() + 200,
                                std::numeric_limits<Int>::max() - 200);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(rowIndexVar, rowIndexVarDist(gen));
      }
      if (randBool()) {
        _solver->setValue(colIndexVar, colIndexVarDist(gen));
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

class MockElement2dVar : public Element2dConst {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Element2dConst::registerVars();
  }
  explicit MockElement2dVar(SolverBase& solver, VarViewId output,
                            VarViewId index1, VarViewId index2,
                            std::vector<std::vector<Int>>&& parMatrix,
                            Int offset1, Int offset2)
      : Element2dConst(solver, output, index1, index2, std::move(parMatrix),
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
    for (size_t i = 0; i < static_cast<size_t>(numRows); ++i) {
      for (size_t j = 0; j < static_cast<size_t>(numCols); ++j) {
        parMatrix.at(i).at(j) = static_cast<Int>(i * numCols + j);
      }
    }
    VarViewId index1 = _solver->makeIntVar(1, 1, numRows);
    VarViewId index2 = _solver->makeIntVar(1, 1, numCols);
    VarViewId output = _solver->makeIntVar(-10, -100, 100);
    testNotifications<MockElement2dVar>(
        &_solver->makeInvariant<MockElement2dVar>(
            *_solver, output, index1, index2, std::move(parMatrix), 1, 1),
        {propMode, markingMode, 3, index1, 5, output});
  }
}

}  // namespace atlantis::testing
