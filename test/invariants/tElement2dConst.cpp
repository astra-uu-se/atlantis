
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/element2dConst.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

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

  Int computeOutput(const Timestamp ts, const VarId rowIndex,
                    const VarId colIndex, const Int rowOffset,
                    const Int colOffset) {
    return computeOutput(engine->value(ts, rowIndex),
                         engine->value(ts, colIndex), rowOffset, colOffset);
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

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = engine->makeInvariant<Element2dConst>(
        outputId, rowIndex, colIndex, matrix, rowOffset, colOffset);
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
                minVal =
                    std::min(minVal, computeOutput(rowIndexVal, colIndexVal,
                                                   rowOffset, colOffset));
                maxVal =
                    std::max(maxVal, computeOutput(rowIndexVal, colIndexVal,
                                                   rowOffset, colOffset));
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

TEST_F(Element2dConstTest, Recompute) {
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

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = engine->makeInvariant<Element2dConst>(
        outputId, rowIndex, colIndex, matrix, rowOffset, colOffset);
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

TEST_F(Element2dConstTest, NotifyInputChanged) {
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

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = engine->makeInvariant<Element2dConst>(
        outputId, rowIndex, colIndex, matrix, rowOffset, colOffset);
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

TEST_F(Element2dConstTest, NextInput) {
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

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = engine->makeInvariant<Element2dConst>(
        outputId, rowIndex, colIndex, matrix, rowOffset, colOffset);
    engine->close();

    for (Timestamp ts = engine->currentTimestamp() + 1;
         ts < engine->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), rowIndex);
      EXPECT_EQ(invariant.nextInput(ts, *engine), colIndex);
      EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    }
  }
}

TEST_F(Element2dConstTest, NotifyCurrentInputChanged) {
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

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = engine->makeInvariant<Element2dConst>(
        outputId, rowIndex, colIndex, matrix, rowOffset, colOffset);
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
      }
    }
  }
}

TEST_F(Element2dConstTest, Commit) {
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

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    Element2dConst& invariant = engine->makeInvariant<Element2dConst>(
        outputId, rowIndex, colIndex, matrix, rowOffset, colOffset);
    engine->close();

    Int committedRowIndexValue = engine->committedValue(rowIndex);
    Int committedColIndexValue = engine->committedValue(colIndex);

    for (size_t i = 0; i < rowIndexValues.size(); ++i) {
      const Int rowIndexVal = rowIndexValues.at(i);
      for (size_t j = 0; j < colIndexValues.size(); ++j) {
        const Int colIndexVal = colIndexValues.at(j);

        const Timestamp ts = engine->currentTimestamp() +
                             Timestamp(i * colIndexValues.size() + j);

        ASSERT_EQ(engine->committedValue(rowIndex), committedRowIndexValue);
        ASSERT_EQ(engine->committedValue(colIndex), committedColIndexValue);

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

        engine->commitIf(ts, rowIndex);
        committedRowIndexValue = engine->value(ts, rowIndex);
        engine->commitIf(ts, colIndex);
        committedColIndexValue = engine->value(ts, colIndex);
        engine->commitIf(ts, outputId);

        invariant.commit(ts, *engine);
        invariant.recompute(ts + 1, *engine);
        ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
      }
    }
  }
}

class MockElement2dVar : public Element2dConst {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    Element2dConst::registerVars(engine);
  }
  explicit MockElement2dVar(VarId output, VarId index1, VarId index2,
                            const std::vector<std::vector<Int>>& matrix,
                            Int offset1, Int offset2)
      : Element2dConst(output, index1, index2, matrix, offset1, offset2) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Element2dConst::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return Element2dConst::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          Element2dConst::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          Element2dConst::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      Element2dConst::commit(t, engine);
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
TEST_F(Element2dConstTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<std::vector<Int>> parMatrix(numRows, std::vector<Int>(numCols));
    for (size_t i = 0; i < numRows; ++i) {
      for (size_t j = 0; j < numCols; ++j) {
        parMatrix.at(i).at(j) = i * numCols + j;
      }
    }
    VarId index1 = engine->makeIntVar(1, 1, numRows);
    VarId index2 = engine->makeIntVar(1, 1, numCols);
    VarId output = engine->makeIntVar(-10, -100, 100);
    testNotifications<MockElement2dVar>(
        &engine->makeInvariant<MockElement2dVar>(output, index1, index2,
                                                 parMatrix, 1, 1),
        propMode, markingMode, 3, index1, 5, output);
  }
}

}  // namespace
