
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/element2dVar.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class Element2dVarTest : public InvariantTest {
 protected:
  const size_t numRows = 4;
  const size_t numCols = 5;
  const Int inputLb = std::numeric_limits<Int>::min();
  const Int inputUb = std::numeric_limits<Int>::max();
  std::vector<std::pair<Int, Int>> offsets =
      cartesianProduct(std::vector<Int>{-10, 0, 10});
  std::vector<std::vector<VarId>> inputMatrix;
  std::uniform_int_distribution<Int> inputDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    inputMatrix.resize(numRows, std::vector<VarId>(numCols, NULL_ID));
    inputDist = std::uniform_int_distribution<Int>(inputLb, inputUb);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputMatrix.clear();
  }

  size_t zeroBasedRowIndex(const Int rowIndexVal, const Int rowOffset) {
    EXPECT_LE(rowOffset, rowIndexVal);
    EXPECT_LT(rowIndexVal - rowOffset, static_cast<Int>(numRows));
    return rowIndexVal - rowOffset;
  }

  size_t zeroBasedColIndex(const Int colIndexVal, const Int colOffset) {
    EXPECT_LE(colOffset, colIndexVal);
    EXPECT_LT(colIndexVal - colOffset, static_cast<Int>(numCols));
    return colIndexVal - colOffset;
  }

  VarId getInput(const Int rowIndexVal, const Int colIndexVal,
                 const Int rowOffset, const Int colOffset) {
    return inputMatrix.at(zeroBasedRowIndex(rowIndexVal, rowOffset))
        .at(zeroBasedColIndex(colIndexVal, colOffset));
  }

  Int computeOutput(const Timestamp ts, const VarId rowIndex,
                    const VarId colIndex, const Int rowOffset,
                    const Int colOffset) {
    return computeOutput(ts, engine->value(ts, rowIndex),
                         engine->value(ts, colIndex), rowOffset, colOffset);
  }

  Int computeOutput(const Timestamp ts, const Int rowIndexVal,
                    const Int colIndexVal, const Int rowOffset,
                    const Int colOffset) {
    return engine->value(
        ts, getInput(rowIndexVal, colIndexVal, rowOffset, colOffset));
  }
};

TEST_F(Element2dVarTest, UpdateBounds) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = numRows - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = numCols - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    engine->open();
    const VarId rowIndex =
        engine->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarId colIndex =
        engine->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            engine->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = engine->makeInvariant<Element2dVar>(
        outputId, rowIndex, colIndex, inputMatrix, rowOffset, colOffset);
    engine->close();

    for (Int minRowIndex = rowIndexLb; minRowIndex <= rowIndexUb;
         ++minRowIndex) {
      for (Int maxRowIndex = rowIndexUb; maxRowIndex >= minRowIndex;
           --maxRowIndex) {
        engine->updateBounds(rowIndex, minRowIndex, maxRowIndex, false);
        for (Int minColIndex = colIndexLb; minColIndex <= colIndexUb;
             ++minColIndex) {
          for (Int maxColIndex = colIndexUb; maxColIndex >= minColIndex;
               --maxColIndex) {
            engine->updateBounds(colIndex, minColIndex, maxColIndex, false);
            invariant.updateBounds(*engine);
            Int minVal = std::numeric_limits<Int>::max();
            Int maxVal = std::numeric_limits<Int>::min();
            for (Int rowIndexVal = minRowIndex; rowIndexVal <= maxRowIndex;
                 ++rowIndexVal) {
              for (Int colIndexVal = minColIndex; colIndexVal <= maxColIndex;
                   ++colIndexVal) {
                minVal = std::min(minVal, engine->lowerBound(
                                              getInput(rowIndexVal, colIndexVal,
                                                       rowOffset, colOffset)));
                maxVal = std::max(maxVal, engine->upperBound(
                                              getInput(rowIndexVal, colIndexVal,
                                                       rowOffset, colOffset)));
              }
            }
            EXPECT_EQ(minVal, engine->lowerBound(outputId));
            EXPECT_EQ(maxVal, engine->upperBound(outputId));
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
    const Int rowIndexUb = numRows - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = numCols - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    engine->open();
    const VarId rowIndex =
        engine->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarId colIndex =
        engine->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            engine->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = engine->makeInvariant<Element2dVar>(
        outputId, rowIndex, colIndex, inputMatrix, rowOffset, colOffset);
    engine->close();

    for (Int rowIndexVal = rowIndexLb; rowIndexVal <= rowIndexUb;
         ++rowIndexVal) {
      engine->setValue(engine->currentTimestamp(), rowIndex, rowIndexVal);
      EXPECT_EQ(engine->value(engine->currentTimestamp(), rowIndex),
                rowIndexVal);
      for (Int colIndexVal = colIndexLb; colIndexVal <= colIndexUb;
           ++colIndexVal) {
        engine->setValue(engine->currentTimestamp(), colIndex, colIndexVal);
        EXPECT_EQ(engine->value(engine->currentTimestamp(), colIndex),
                  colIndexVal);

        const Int expectedOutput =
            computeOutput(engine->currentTimestamp(), rowIndex, colIndex,
                          rowOffset, colOffset);
        invariant.recompute(engine->currentTimestamp(), *engine);
        EXPECT_EQ(engine->value(engine->currentTimestamp(), rowIndex),
                  rowIndexVal);

        EXPECT_EQ(expectedOutput,
                  engine->value(engine->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(Element2dVarTest, NotifyInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = numRows - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = numCols - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    engine->open();
    const VarId rowIndex =
        engine->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarId colIndex =
        engine->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            engine->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = engine->makeInvariant<Element2dVar>(
        outputId, rowIndex, colIndex, inputMatrix, rowOffset, colOffset);
    engine->close();

    for (Int rowIndexVal = rowIndexLb; rowIndexVal <= rowIndexUb;
         ++rowIndexVal) {
      engine->setValue(engine->currentTimestamp(), rowIndex, rowIndexVal);
      for (Int colIndexVal = colIndexLb; colIndexVal <= colIndexUb;
           ++colIndexVal) {
        engine->setValue(engine->currentTimestamp(), colIndex, colIndexVal);

        const Int expectedOutput =
            computeOutput(engine->currentTimestamp(), rowIndex, colIndex,
                          rowOffset, colOffset);

        invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                     LocalId(0));
        EXPECT_EQ(expectedOutput,
                  engine->value(engine->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(Element2dVarTest, NextInput) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = numRows - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = numCols - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    engine->open();
    const VarId rowIndex =
        engine->makeIntVar(rowIndexDist(gen), rowIndexLb, rowIndexUb);

    const VarId colIndex =
        engine->makeIntVar(colIndexDist(gen), colIndexLb, colIndexUb);

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            engine->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = engine->makeInvariant<Element2dVar>(
        outputId, rowIndex, colIndex, inputMatrix, rowOffset, colOffset);
    engine->close();

    for (Timestamp ts = engine->currentTimestamp() + 1;
         ts < engine->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), rowIndex);
      EXPECT_EQ(invariant.nextInput(ts, *engine), colIndex);
      EXPECT_EQ(invariant.nextInput(ts, *engine),
                getInput(engine->value(ts, rowIndex),
                         engine->value(ts, colIndex), rowOffset, colOffset));
      EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    }
  }
}

TEST_F(Element2dVarTest, NotifyCurrentInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
  Timestamp t0 = engine->currentTimestamp() +
                 (numRows * numCols * static_cast<Int>(offsets.size())) + 1;
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = numRows - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::vector<Int> rowIndexValues(numRows, 0);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = numCols - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    engine->open();
    const VarId rowIndex =
        engine->makeIntVar(rowIndexValues.back(), rowIndexLb, rowIndexUb);

    const VarId colIndex =
        engine->makeIntVar(colIndexValues.back(), colIndexLb, colIndexUb);

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            engine->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = engine->makeInvariant<Element2dVar>(
        outputId, rowIndex, colIndex, inputMatrix, rowOffset, colOffset);
    engine->close();

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);
        const Timestamp ts = t0 + Timestamp(i * colIndexValues.size() + j);

        EXPECT_EQ(invariant.nextInput(ts, *engine), rowIndex);
        engine->setValue(ts, rowIndex, rowIndexVal);
        invariant.notifyCurrentInputChanged(ts, *engine);

        EXPECT_EQ(invariant.nextInput(ts, *engine), colIndex);
        engine->setValue(ts, colIndex, colIndexVal);
        invariant.notifyCurrentInputChanged(ts, *engine);

        EXPECT_EQ(engine->value(ts, outputId),
                  computeOutput(ts, rowIndex, colIndex, rowOffset, colOffset));

        const VarId curInput = invariant.nextInput(ts, *engine);
        EXPECT_EQ(curInput,
                  getInput(rowIndexVal, colIndexVal, rowOffset, colOffset));

        const Int oldInputVal = engine->value(ts, curInput);
        do {
          engine->setValue(ts, curInput, inputDist(gen));
        } while (engine->value(ts, curInput) == oldInputVal);

        invariant.notifyCurrentInputChanged(ts, *engine);
        EXPECT_EQ(engine->value(ts, outputId),
                  computeOutput(ts, rowIndex, colIndex, rowOffset, colOffset));
      }
    }
  }
}

TEST_F(Element2dVarTest, Commit) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const auto& [rowOffset, colOffset] : offsets) {
    const Int rowIndexLb = rowOffset;
    const Int rowIndexUb = numRows - 1 + rowOffset;
    EXPECT_TRUE(rowIndexLb <= rowIndexUb);

    std::vector<Int> rowIndexValues(numRows);
    std::iota(rowIndexValues.begin(), rowIndexValues.end(), rowOffset);
    std::shuffle(rowIndexValues.begin(), rowIndexValues.end(), rng);

    std::uniform_int_distribution<Int> rowIndexDist(rowIndexLb, rowIndexUb);

    const Int colIndexLb = colOffset;
    const Int colIndexUb = numCols - 1 + colOffset;
    EXPECT_TRUE(colIndexLb <= colIndexUb);

    std::vector<Int> colIndexValues(numCols, 0);
    std::iota(colIndexValues.begin(), colIndexValues.end(), colOffset);
    std::shuffle(colIndexValues.begin(), colIndexValues.end(), rng);

    std::uniform_int_distribution<Int> colIndexDist(colIndexLb, colIndexUb);

    engine->open();
    const VarId rowIndex =
        engine->makeIntVar(rowIndexValues.back(), rowIndexLb, rowIndexUb);

    const VarId colIndex =
        engine->makeIntVar(colIndexValues.back(), colIndexLb, colIndexUb);

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        inputMatrix.at(i).at(j) =
            engine->makeIntVar(inputDist(gen), inputLb, inputUb);
      }
    }

    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    Element2dVar& invariant = engine->makeInvariant<Element2dVar>(
        outputId, rowIndex, colIndex, inputMatrix, rowOffset, colOffset);
    engine->close();

    Int committedRowIndexValue = engine->committedValue(rowIndex);
    Int committedColIndexValue = engine->committedValue(colIndex);

    std::vector<std::vector<Int>> committedInputValues(
        numRows, std::vector<Int>(numCols));

    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        committedInputValues.at(i).at(j) =
            engine->committedValue(inputMatrix.at(i).at(j));
      }
    }

    Timestamp ts = engine->currentTimestamp();
    for (const Int rowIndexVal : rowIndexValues) {
      for (const Int colIndexVal : colIndexValues) {
        ++ts;

        ASSERT_EQ(engine->committedValue(rowIndex), committedRowIndexValue);
        ASSERT_EQ(engine->committedValue(colIndex), committedColIndexValue);

        for (size_t i = 0; i < numRows; ++i) {
          for (size_t j = 0; j < numCols; ++j) {
            ASSERT_EQ(engine->committedValue(inputMatrix.at(i).at(j)),
                      committedInputValues.at(i).at(j));
          }
        }

        // Change row index
        engine->setValue(ts, rowIndex, rowIndexVal);

        // notify row index change
        invariant.notifyInputChanged(ts, *engine, LocalId(0));

        // incremental value from row index
        Int notifiedOutput = engine->value(ts, outputId);
        invariant.recompute(ts, *engine);

        ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

        // Change col index
        engine->setValue(ts, colIndex, colIndexVal);

        // notify col index change
        invariant.notifyInputChanged(ts, *engine, LocalId(0));

        // incremental value from col index
        notifiedOutput = engine->value(ts, outputId);
        invariant.recompute(ts, *engine);

        ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

        // Change input
        const VarId curInput =
            getInput(rowIndexVal, colIndexVal, rowOffset, colOffset);
        const Int oldInputVal = engine->value(ts, curInput);
        do {
          engine->setValue(ts, curInput, inputDist(gen));
        } while (engine->value(ts, curInput) == oldInputVal);

        // notify input change
        invariant.notifyInputChanged(ts, *engine, LocalId(0));

        // incremental value from input
        notifiedOutput = engine->value(ts, outputId);
        invariant.recompute(ts, *engine);

        ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

        engine->commitIf(ts, rowIndex);
        committedRowIndexValue = engine->value(ts, rowIndex);
        engine->commitIf(ts, colIndex);
        committedColIndexValue = engine->value(ts, colIndex);
        engine->commitIf(ts, curInput);
        committedInputValues.at(zeroBasedRowIndex(rowIndexVal, rowOffset))
            .at(zeroBasedColIndex(colIndexVal, colOffset)) =
            engine->value(ts, curInput);
        engine->commitIf(ts, outputId);

        invariant.commit(ts, *engine);
        invariant.recompute(ts + 1, *engine);
        ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
      }
    }
  }
}

class MockElement2dVar : public Element2dVar {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    Element2dVar::registerVars(engine);
  }
  explicit MockElement2dVar(VarId output, VarId index1, VarId index2,
                            const std::vector<std::vector<VarId>>& varMatrix,
                            Int offset1, Int offset2)
      : Element2dVar(output, index1, index2, varMatrix, offset1, offset2) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Element2dVar::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return Element2dVar::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          Element2dVar::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          Element2dVar::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      Element2dVar::commit(t, engine);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));
  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp t, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
};
TEST_F(Element2dVarTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<std::vector<VarId>> varMatrix(
        numRows, std::vector<VarId>(numCols, NULL_ID));
    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        varMatrix.at(i).at(j) = engine->makeIntVar(i * numCols + j, -100, 100);
      }
    }
    VarId index1 = engine->makeIntVar(1, 1, numRows);
    VarId index2 = engine->makeIntVar(1, 1, numCols);
    VarId output = engine->makeIntVar(-10, -100, 100);
    testNotifications<MockElement2dVar>(
        &engine->makeInvariant<MockElement2dVar>(output, index1, index2,
                                                 varMatrix, 1, 1),
        propMode, markingMode, 4, index1, 5, output);
  }
}

}  // namespace
