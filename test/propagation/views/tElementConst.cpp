#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/elementConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ElementConstTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;
  std::mt19937 gen;
  std::default_random_engine rng;

  const size_t numValues = 1000;
  const Int valueLb = std::numeric_limits<Int>::min();
  const Int valueUb = std::numeric_limits<Int>::max();
  Int indexLb = 1;
  Int indexUb = static_cast<Int>(numValues);
  std::vector<Int> values;
  std::uniform_int_distribution<Int> valueDist;
  std::uniform_int_distribution<Int> indexDist;

 public:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    _solver = std::make_shared<Solver>();

    values.resize(numValues, 0);
    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    indexDist = std::uniform_int_distribution<Int>(indexLb, indexUb);
    for (long& value : values) {
      value = valueDist(gen);
    }
  }

  void TearDown() override { values.clear(); }

  Int computeOutput(const Timestamp ts, const VarViewId index) {
    return computeOutput(_solver->value(ts, index));
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

  _solver->open();
  const VarViewId index = _solver->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarViewId outputId = _solver->makeIntView<ElementConst>(
      *_solver, index, std::vector<Int>(values));
  _solver->close();

  const Int ub = 100;

  for (Int minIndex = indexLb; minIndex <= ub; ++minIndex) {
    for (Int maxIndex = ub; maxIndex >= minIndex; --maxIndex) {
      _solver->updateBounds(VarId(index), minIndex, maxIndex, false);
      Int minVal = std::numeric_limits<Int>::max();
      Int maxVal = std::numeric_limits<Int>::min();
      for (Int i = minIndex - 1; i < maxIndex; ++i) {
        minVal = std::min(minVal, values.at(i));
        maxVal = std::max(maxVal, values.at(i));
      }
      EXPECT_EQ(minVal, _solver->lowerBound(outputId));
      EXPECT_EQ(maxVal, _solver->upperBound(outputId));
    }
  }
}

TEST_F(ElementConstTest, Value) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  _solver->open();
  const VarViewId index = _solver->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarViewId outputId = _solver->makeIntView<ElementConst>(
      *_solver, index, std::vector<Int>(values));
  _solver->close();

  for (Int val = indexLb; val <= indexUb; ++val) {
    _solver->setValue(_solver->currentTimestamp(), index, val);
    EXPECT_EQ(_solver->value(_solver->currentTimestamp(), index), val);
    const Int expectedOutput =
        computeOutput(_solver->currentTimestamp(), index);

    EXPECT_EQ(expectedOutput,
              _solver->value(_solver->currentTimestamp(), outputId));
  }
}

TEST_F(ElementConstTest, CommittedValue) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  std::vector<Int> indexValues(numValues);
  std::iota(indexValues.begin(), indexValues.end(), 1);
  std::shuffle(indexValues.begin(), indexValues.end(), rng);

  _solver->open();
  const VarViewId index = _solver->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarViewId outputId = _solver->makeIntView<ElementConst>(
      *_solver, index, std::vector<Int>(values));
  _solver->close();

  Int committedValue = _solver->committedValue(index);

  for (size_t i = 0; i < indexValues.size(); ++i) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(1 + i);
    ASSERT_EQ(_solver->committedValue(index), committedValue);

    _solver->setValue(ts, index, indexValues[i]);

    const Int expectedOutput = computeOutput(indexValues[i]);

    ASSERT_EQ(expectedOutput, _solver->value(ts, outputId));

    _solver->commitIf(ts, VarId(index));
    committedValue = _solver->value(ts, index);

    ASSERT_EQ(expectedOutput, _solver->value(ts + 1, outputId));
  }
}

}  // namespace atlantis::testing
