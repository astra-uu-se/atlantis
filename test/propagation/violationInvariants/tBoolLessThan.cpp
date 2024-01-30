#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/violationInvariants/boolLessThan.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolLessThanTest : public InvariantTest {
 public:
  bool isRegistered = false;

  Int computeViolation(const Timestamp ts,
                       const std::array<const VarId, 2>& inputs) {
    return computeViolation(solver->value(ts, inputs.at(0)),
                            solver->value(ts, inputs.at(1)));
  }

  static Int computeViolation(const std::array<const Int, 2>& inputs) {
    return computeViolation(inputs.at(0), inputs.at(1));
  }

  Int computeViolation(const Timestamp ts, const VarId x, const VarId y) {
    return computeViolation(solver->value(ts, x), solver->value(ts, y));
  }

  static Int computeViolation(const Int xVal, const Int yVal) {
    // both are violating:
    if (xVal != 0 && yVal != 0) {
      // y must be satisfied
      return yVal;
    }
    // x is violating and y is satisfied:
    if (xVal != 0 && yVal == 0) {
      // satisfied
      return 0;
    }
    // x is satisfied and y is violating:
    if (xVal == 0 && yVal != 0) {
      // x must violate and y must be satisfied
      return 1 + yVal;
    }
    // both are satisfied
    if (xVal == 0 && yVal == 0) {
      // x must violate
      return 1;
    }
    EXPECT_FALSE(true);
    return -1;
  }
};

TEST_F(BoolLessThanTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 10}, {1, 10}, {10, 100}};
  solver->open();
  const VarId x = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId violationId = solver->makeIntVar(0, 0, 2);
  BoolLessThan& invariant =
      solver->makeViolationInvariant<BoolLessThan>(*solver, violationId, x, y);
  solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    solver->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      solver->updateBounds(y, yLb, yUb, false);
      invariant.updateBounds(false);
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        solver->setValue(solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          solver->setValue(solver->currentTimestamp(), y, yVal);
          invariant.updateBounds(false);
          invariant.recompute(solver->currentTimestamp());
        }
      }
      ASSERT_GE(0, solver->lowerBound(violationId));
      ASSERT_LE(1, solver->upperBound(violationId));
    }
  }
}

TEST_F(BoolLessThanTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 100;
  const Int yLb = 0;
  const Int yUb = 100;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  solver->open();
  const std::array<const VarId, 2> inputs{solver->makeIntVar(xUb, xLb, xUb),
                                          solver->makeIntVar(yUb, yLb, yUb)};
  const VarId violationId =
      solver->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  BoolLessThan& invariant = solver->makeViolationInvariant<BoolLessThan>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      solver->setValue(solver->currentTimestamp(), inputs.at(0), xVal);
      solver->setValue(solver->currentTimestamp(), inputs.at(1), yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedViolation,
                solver->value(solver->currentTimestamp(), violationId));
    }
  }
}

TEST_F(BoolLessThanTest, NotifyInputChanged) {
  const Int lb = 0;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<const VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                                          solver->makeIntVar(ub, lb, ub)};
  const VarId violationId = solver->makeIntVar(0, 0, ub - lb);
  BoolLessThan& invariant = solver->makeViolationInvariant<BoolLessThan>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      solver->setValue(ts, inputs.at(i), val);
      const Int expectedViolation = computeViolation(ts, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedViolation, solver->value(ts, violationId));
    }
  }
}

TEST_F(BoolLessThanTest, NextInput) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<const VarId, 2> inputs = {solver->makeIntVar(0, lb, ub),
                                             solver->makeIntVar(1, lb, ub)};
  const VarId violationId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  BoolLessThan& invariant = solver->makeViolationInvariant<BoolLessThan>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
      const VarId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(BoolLessThanTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<const VarId, 2> inputs = {
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId violationId = solver->makeIntVar(0, 0, ub - lb);
  BoolLessThan& invariant = solver->makeViolationInvariant<BoolLessThan>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = solver->value(ts, varId);
      do {
        solver->setValue(ts, varId, valueDist(gen));
      } while (solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, violationId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(BoolLessThanTest, Commit) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::shuffle(indices.begin(), indices.end(), rng);

  solver->open();
  const std::array<const VarId, 2> inputs{
      solver->makeIntVar(committedValues.at(0), lb, ub),
      solver->makeIntVar(committedValues.at(1), lb, ub)};

  const VarId violationId = solver->makeIntVar(0, 0, 2);
  BoolLessThan& invariant = solver->makeViolationInvariant<BoolLessThan>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  EXPECT_EQ(solver->value(solver->currentTimestamp(), violationId),
            computeViolation(solver->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputs.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      solver->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == solver->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedViolation = solver->value(ts, violationId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, solver->value(ts, violationId));

    solver->commitIf(ts, inputs.at(i));
    committedValues.at(i) = solver->value(ts, inputs.at(i));
    solver->commitIf(ts, violationId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, solver->value(ts + 1, violationId));
  }
}

class MockBoolLessThan : public BoolLessThan {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolLessThan::registerVars();
  }
  explicit MockBoolLessThan(SolverBase& solver, VarId violationId, VarId x, VarId y)
      : BoolLessThan(solver, violationId, x, y) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolLessThan::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolLessThan::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolLessThan::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolLessThan::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolLessThan::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BoolLessThanTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId x = solver->makeIntVar(5, 0, 100);
    const VarId y = solver->makeIntVar(0, 0, 100);
    const VarId viol = solver->makeIntVar(0, 0, 200);
    testNotifications<MockBoolLessThan>(
        &solver->makeViolationInvariant<MockBoolLessThan>(*solver, viol, x, y),
        {propMode, markingMode, 3, x, 1, viol});
  }
}
}  // namespace atlantis::testing
