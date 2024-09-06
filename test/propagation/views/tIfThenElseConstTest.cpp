#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IfThenElseConstTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;
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
    _solver = std::make_unique<Solver>();

    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    thenVal = valueDist(gen);
    elseVal = valueDist(gen);
  }

  Int computeOutput(const Timestamp ts, const VarViewId condVarId) {
    return computeOutput(_solver->value(ts, condVarId));
  }

  Int computeOutput(const Int conditionVal) {
    EXPECT_GE(conditionVal, 0);
    return conditionVal == 0 ? thenVal : elseVal;
  }
};

TEST_F(IfThenElseConstTest, Bounds) {
  _solver->open();
  const VarViewId condVarId = _solver->makeIntVar(condLb, condLb, condUb);
  const VarViewId outputId = _solver->makeIntView<IfThenElseConst>(
      *_solver, condVarId, thenVal, elseVal);
  _solver->close();

  EXPECT_EQ(std::min(thenVal, elseVal), _solver->lowerBound(outputId));
  EXPECT_EQ(std::max(thenVal, elseVal), _solver->upperBound(outputId));

  _solver->updateBounds(VarId(condVarId), 0, 0, false);
  EXPECT_EQ(thenVal, _solver->lowerBound(outputId));
  EXPECT_EQ(thenVal, _solver->upperBound(outputId));

  _solver->updateBounds(VarId(condVarId), 1, 1, false);
  EXPECT_EQ(elseVal, _solver->lowerBound(outputId));
  EXPECT_EQ(elseVal, _solver->upperBound(outputId));
}

TEST_F(IfThenElseConstTest, Value) {
  _solver->open();
  const VarViewId condVarId = _solver->makeIntVar(condLb, condLb, condUb);
  const VarViewId outputId = _solver->makeIntView<IfThenElseConst>(
      *_solver, condVarId, thenVal, elseVal);
  _solver->close();

  for (Int condVal = condLb; condVal <= condUb; ++condVal) {
    _solver->setValue(_solver->currentTimestamp(), condVarId, condVal);
    EXPECT_EQ(_solver->value(_solver->currentTimestamp(), condVarId), condVal);
    const Int expectedOutput =
        computeOutput(_solver->currentTimestamp(), condVarId);

    EXPECT_EQ(expectedOutput,
              _solver->value(_solver->currentTimestamp(), outputId));
  }
}

TEST_F(IfThenElseConstTest, CommittedValue) {
  std::vector<Int> condValues{0, 1};
  std::shuffle(condValues.begin(), condValues.end(), rng);

  _solver->open();
  const VarViewId condVarId = _solver->makeIntVar(condLb, condLb, condUb);
  const VarViewId outputId = _solver->makeIntView<IfThenElseConst>(
      *_solver, condVarId, thenVal, elseVal);
  _solver->close();

  Int committedValue = _solver->committedValue(condVarId);

  for (size_t i = 0; i < condValues.size(); ++i) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(1 + i);
    ASSERT_EQ(_solver->committedValue(condVarId), committedValue);

    _solver->setValue(ts, condVarId, condValues[i]);

    const Int expectedOutput = computeOutput(condValues[i]);

    ASSERT_EQ(expectedOutput, _solver->value(ts, outputId));

    _solver->commitIf(ts, VarId(condVarId));
    committedValue = _solver->value(ts, condVarId);

    ASSERT_EQ(expectedOutput, _solver->value(ts + 1, outputId));
  }
}

}  // namespace atlantis::testing
