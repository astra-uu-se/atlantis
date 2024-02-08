#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/elementConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ElementConstTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;
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
    solver = std::make_unique<Solver>();

    values.resize(numValues, 0);
    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    indexDist = std::uniform_int_distribution<Int>(indexLb, indexUb);
    for (long& value : values) {
      value = valueDist(gen);
    }
  }

  void TearDown() override { values.clear(); }

  Int computeOutput(const Timestamp ts, const VarId index) {
    return computeOutput(solver->value(ts, index));
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

  solver->open();
  const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = solver->makeIntView<ElementConst>(
      *solver, index, std::vector<Int>(values));
  solver->close();

  const Int ub = 100;

  for (Int minIndex = indexLb; minIndex <= ub; ++minIndex) {
    for (Int maxIndex = ub; maxIndex >= minIndex; --maxIndex) {
      solver->updateBounds(index, minIndex, maxIndex, false);
      Int minVal = std::numeric_limits<Int>::max();
      Int maxVal = std::numeric_limits<Int>::min();
      for (Int i = minIndex - 1; i < maxIndex; ++i) {
        minVal = std::min(minVal, values.at(i));
        maxVal = std::max(maxVal, values.at(i));
      }
      EXPECT_EQ(minVal, solver->lowerBound(outputId));
      EXPECT_EQ(maxVal, solver->upperBound(outputId));
    }
  }
}

TEST_F(ElementConstTest, Value) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  solver->open();
  const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = solver->makeIntView<ElementConst>(
      *solver, index, std::vector<Int>(values));
  solver->close();

  for (Int val = indexLb; val <= indexUb; ++val) {
    solver->setValue(solver->currentTimestamp(), index, val);
    EXPECT_EQ(solver->value(solver->currentTimestamp(), index), val);
    const Int expectedOutput = computeOutput(solver->currentTimestamp(), index);

    EXPECT_EQ(expectedOutput,
              solver->value(solver->currentTimestamp(), outputId));
  }
}

TEST_F(ElementConstTest, CommittedValue) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  std::vector<Int> indexValues(numValues);
  std::iota(indexValues.begin(), indexValues.end(), 1);
  std::shuffle(indexValues.begin(), indexValues.end(), rng);

  solver->open();
  const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = solver->makeIntView<ElementConst>(
      *solver, index, std::vector<Int>(values));
  solver->close();

  Int committedValue = solver->committedValue(index);

  for (size_t i = 0; i < indexValues.size(); ++i) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(1 + i);
    ASSERT_EQ(solver->committedValue(index), committedValue);

    solver->setValue(ts, index, indexValues[i]);

    const Int expectedOutput = computeOutput(indexValues[i]);

    ASSERT_EQ(expectedOutput, solver->value(ts, outputId));

    solver->commitIf(ts, index);
    committedValue = solver->value(ts, index);

    ASSERT_EQ(expectedOutput, solver->value(ts + 1, outputId));
  }
}

}  // namespace atlantis::testing
