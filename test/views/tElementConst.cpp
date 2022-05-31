
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "views/elementConst.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class ElementConstTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  std::mt19937 gen;
  std::default_random_engine rng;

  const size_t numValues = 1000;
  const Int valueLb = std::numeric_limits<Int>::min();
  const Int valueUb = std::numeric_limits<Int>::max();
  Int indexLb = 1;
  Int indexUb = numValues;
  std::vector<Int> values;
  std::uniform_int_distribution<Int> valueDist;
  std::uniform_int_distribution<Int> indexDist;

 public:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();

    values.resize(numValues, 0);
    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    indexDist = std::uniform_int_distribution<Int>(indexLb, indexUb);
    for (size_t i = 0; i < values.size(); ++i) {
      values.at(i) = valueDist(gen);
    }
  }

  void TearDown() override { values.clear(); }

  Int computeOutput(const Timestamp ts, const VarId index) {
    return computeOutput(engine->value(ts, index));
  }

  Int computeOutput(const Int indexVal) {
    EXPECT_TRUE(1 <= indexVal);
    EXPECT_TRUE(static_cast<size_t>(indexVal) <= values.size());
    return values.at(indexVal - 1);
  }
};

TEST_F(ElementConstTest, Bounds) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntView<ElementConst>(index, values);
  engine->close();

  const Int ub = 100;

  for (Int minIndex = indexLb; minIndex <= ub; ++minIndex) {
    for (Int maxIndex = ub; maxIndex >= minIndex; --maxIndex) {
      engine->updateBounds(index, minIndex, maxIndex, false);
      Int minVal = std::numeric_limits<Int>::max();
      Int maxVal = std::numeric_limits<Int>::min();
      for (Int i = minIndex - 1; i < maxIndex; ++i) {
        minVal = std::min(minVal, values.at(i));
        maxVal = std::max(maxVal, values.at(i));
      }
      EXPECT_EQ(minVal, engine->lowerBound(outputId));
      EXPECT_EQ(maxVal, engine->upperBound(outputId));
    }
  }
}

TEST_F(ElementConstTest, Value) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntView<ElementConst>(index, values);
  engine->close();

  for (Int val = indexLb; val <= indexUb; ++val) {
    engine->setValue(engine->currentTimestamp(), index, val);
    EXPECT_EQ(engine->value(engine->currentTimestamp(), index), val);
    const Int expectedOutput = computeOutput(engine->currentTimestamp(), index);

    EXPECT_EQ(expectedOutput,
              engine->value(engine->currentTimestamp(), outputId));
  }
}

TEST_F(ElementConstTest, CommittedValue) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  std::vector<Int> indexValues(numValues, 0);
  for (size_t i = 0; i < indexValues.size(); ++i) {
    indexValues.at(i) = i + 1;
  }

  std::shuffle(indexValues.begin(), indexValues.end(), rng);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntView<ElementConst>(index, values);
  engine->close();

  Int committedValue = engine->committedValue(index);

  for (size_t i = 0; i < indexValues.size(); ++i) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(1 + i);
    ASSERT_EQ(engine->committedValue(index), committedValue);

    engine->setValue(ts, index, indexValues[i]);

    const Int expectedOutput = computeOutput(indexValues[i]);

    ASSERT_EQ(expectedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, index);
    committedValue = engine->value(ts, index);

    ASSERT_EQ(expectedOutput, engine->value(ts + 1, outputId));
  }
}

}  // namespace
