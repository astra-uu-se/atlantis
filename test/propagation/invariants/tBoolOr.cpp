#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolOrTest : public InvariantTest {
 public:
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
    return std::min(xVal, yVal);
  }
};

TEST_F(BoolOrTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 10}, {1, 10}, {10, 100}};
  solver->open();
  const VarId x = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  BoolOr& invariant = solver->makeInvariant<BoolOr>(*solver, outputId, x, y);
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
      ASSERT_GE(std::min(xLb, yLb), solver->lowerBound(outputId));
      ASSERT_EQ(std::min(xUb, yUb), solver->upperBound(outputId));
    }
  }
}

TEST_F(BoolOrTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 100;
  const Int yLb = 0;
  const Int yUb = 100;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  solver->open();
  const std::array<const VarId, 2> inputs{solver->makeIntVar(xUb, xLb, xUb),
                                          solver->makeIntVar(yUb, yLb, yUb)};
  const VarId outputId =
      solver->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  BoolOr& invariant = solver->makeInvariant<BoolOr>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
  solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      solver->setValue(solver->currentTimestamp(), inputs.at(0), xVal);
      solver->setValue(solver->currentTimestamp(), inputs.at(1), yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedViolation,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(BoolOrTest, NotifyInputChanged) {
  const Int lb = 0;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<const VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                                          solver->makeIntVar(ub, lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  BoolOr& invariant = solver->makeInvariant<BoolOr>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      solver->setValue(ts, inputs.at(i), val);
      const Int expectedViolation = computeViolation(ts, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedViolation, solver->value(ts, outputId));
    }
  }
}

TEST_F(BoolOrTest, NextInput) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<const VarId, 2> inputs = {solver->makeIntVar(0, lb, ub),
                                             solver->makeIntVar(1, lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  BoolOr& invariant = solver->makeInvariant<BoolOr>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
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

TEST_F(BoolOrTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<const VarId, 2> inputs = {
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  BoolOr& invariant = solver->makeInvariant<BoolOr>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (const VarId& varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = solver->value(ts, varId);
      do {
        solver->setValue(ts, varId, valueDist(gen));
      } while (solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, outputId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(BoolOrTest, Commit) {
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

  const VarId outputId = solver->makeIntVar(0, 0, 2);
  BoolOr& invariant = solver->makeInvariant<BoolOr>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
  solver->close();

  EXPECT_EQ(solver->value(solver->currentTimestamp(), outputId),
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
    const Int notifiedViolation = solver->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, solver->value(ts, outputId));

    solver->commitIf(ts, inputs.at(i));
    committedValues.at(i) = solver->value(ts, inputs.at(i));
    solver->commitIf(ts, outputId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, solver->value(ts + 1, outputId));
  }
}

class MockBoolOr : public BoolOr {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolOr::registerVars();
  }
  explicit MockBoolOr(SolverBase& solver, VarId outputId, VarId x, VarId y)
      : BoolOr(solver, outputId, x, y) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolOr::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolOr::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolOr::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolOr::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolOr::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BoolOrTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId x = solver->makeIntVar(5, 0, 100);
    const VarId y = solver->makeIntVar(0, 0, 100);
    const VarId output = solver->makeIntVar(0, 0, 200);
    testNotifications<MockBoolOr>(
        &solver->makeInvariant<MockBoolOr>(*solver, output, x, y),
        {propMode, markingMode, 3, x, 1, output});
  }
}
}  // namespace atlantis::testing
