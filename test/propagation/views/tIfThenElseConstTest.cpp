#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IfThenElseConstTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;
  std::mt19937 gen;
  std::default_random_engine rng;

  Int thenVal{0};
  Int elseVal{0};
  const Int condLb = 0;
  const Int condUb = 1;
  const Int valueLb = std::numeric_limits<Int>::min();
  const Int valueUb = std::numeric_limits<Int>::max();
  std::uniform_int_distribution<Int> valueDist;

 public:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    solver = std::make_unique<Solver>();

    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    thenVal = valueDist(gen);
    elseVal = valueDist(gen);
  }

  Int computeOutput(const Timestamp ts, const VarId condVarId) {
    return computeOutput(solver->value(ts, condVarId));
  }

  Int computeOutput(const Int conditionVal) {
    EXPECT_GE(conditionVal, 0);
    return conditionVal == 0 ? thenVal : elseVal;
  }
};

TEST_F(IfThenElseConstTest, Bounds) {
  solver->open();
  const VarId condVarId = solver->makeIntVar(condLb, condLb, condUb);
  const VarId outputId = solver->makeIntView<IfThenElseConst>(
      *solver, condVarId, thenVal, elseVal);
  solver->close();

  EXPECT_EQ(std::min(thenVal, elseVal), solver->lowerBound(outputId));
  EXPECT_EQ(std::max(thenVal, elseVal), solver->upperBound(outputId));

  solver->updateBounds(condVarId, 0, 0, false);
  EXPECT_EQ(thenVal, solver->lowerBound(outputId));
  EXPECT_EQ(thenVal, solver->upperBound(outputId));

  solver->updateBounds(condVarId, 1, 1, false);
  EXPECT_EQ(elseVal, solver->lowerBound(outputId));
  EXPECT_EQ(elseVal, solver->upperBound(outputId));
}

TEST_F(IfThenElseConstTest, Value) {
  solver->open();
  const VarId condVarId = solver->makeIntVar(condLb, condLb, condUb);
  const VarId outputId = solver->makeIntView<IfThenElseConst>(
      *solver, condVarId, thenVal, elseVal);
  solver->close();

  for (Int condVal = condLb; condVal <= condUb; ++condVal) {
    solver->setValue(solver->currentTimestamp(), condVarId, condVal);
    EXPECT_EQ(solver->value(solver->currentTimestamp(), condVarId), condVal);
    const Int expectedOutput =
        computeOutput(solver->currentTimestamp(), condVarId);

    EXPECT_EQ(expectedOutput,
              solver->value(solver->currentTimestamp(), outputId));
  }
}

TEST_F(IfThenElseConstTest, CommittedValue) {
  std::vector<Int> condValues{0, 1};
  std::shuffle(condValues.begin(), condValues.end(), rng);

  solver->open();
  const VarId condVarId = solver->makeIntVar(condLb, condLb, condUb);
  const VarId outputId = solver->makeIntView<IfThenElseConst>(
      *solver, condVarId, thenVal, elseVal);
  solver->close();

  Int committedValue = solver->committedValue(condVarId);

  for (size_t i = 0; i < condValues.size(); ++i) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(1 + i);
    ASSERT_EQ(solver->committedValue(condVarId), committedValue);

    solver->setValue(ts, condVarId, condValues[i]);

    const Int expectedOutput = computeOutput(condValues[i]);

    ASSERT_EQ(expectedOutput, solver->value(ts, outputId));

    solver->commitIf(ts, condVarId);
    committedValue = solver->value(ts, condVarId);

    ASSERT_EQ(expectedOutput, solver->value(ts + 1, outputId));
  }
}

}  // namespace atlantis::testing
