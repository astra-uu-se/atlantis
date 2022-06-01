#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/boolLessEqual.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class BoolLessEqualTest : public InvariantTest {
 public:
  bool isRegistered = false;

  Int computeViolation(const Timestamp ts,
                       const std::array<const VarId, 2>& inputs) {
    return computeViolation(engine->value(ts, inputs.at(0)),
                            engine->value(ts, inputs.at(1)));
  }

  Int computeViolation(const std::array<const Int, 2>& inputs) {
    return computeViolation(inputs.at(0), inputs.at(1));
  }

  Int computeViolation(const Timestamp ts, const VarId x, const VarId y) {
    return computeViolation(engine->value(ts, x), engine->value(ts, y));
  }

  Int computeViolation(const Int xVal, const Int yVal) {
    if (xVal == 0 && yVal != 0) {
      return 1;
    }
    return 0;
  }
};

TEST_F(BoolLessEqualTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 10}, {1, 10}, {10, 100}};
  engine->open();
  const VarId x = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId violationId = engine->makeIntVar(0, 0, 2);
  BoolLessEqual& invariant =
      engine->makeConstraint<BoolLessEqual>(violationId, x, y);
  engine->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    engine->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      engine->updateBounds(y, yLb, yUb, false);
      invariant.updateBounds(*engine);
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        engine->setValue(engine->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          engine->setValue(engine->currentTimestamp(), y, yVal);
          invariant.updateBounds(*engine);
          invariant.recompute(engine->currentTimestamp(), *engine);
        }
      }
      ASSERT_GE(0, engine->lowerBound(violationId));
      ASSERT_LE(1, engine->upperBound(violationId));
    }
  }
}

TEST_F(BoolLessEqualTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 100;
  const Int yLb = 0;
  const Int yUb = 100;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  engine->open();
  const std::array<const VarId, 2> inputs{engine->makeIntVar(xUb, xLb, xUb),
                                          engine->makeIntVar(yUb, yLb, yUb)};
  const VarId violationId =
      engine->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  BoolLessEqual& invariant = engine->makeConstraint<BoolLessEqual>(
      violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      engine->setValue(engine->currentTimestamp(), inputs.at(0), xVal);
      engine->setValue(engine->currentTimestamp(), inputs.at(1), yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(BoolLessEqualTest, NotifyInputChanged) {
  const Int lb = 0;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<const VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                                          engine->makeIntVar(ub, lb, ub)};
  const VarId violationId = engine->makeIntVar(0, 0, ub - lb);
  BoolLessEqual& invariant = engine->makeConstraint<BoolLessEqual>(
      violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Int val = lb; val <= ub; ++val) {
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(engine->currentTimestamp(), inputs.at(i), val);
      const Int expectedViolation =
          computeViolation(engine->currentTimestamp(), inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(i));
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(BoolLessEqualTest, NextInput) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<const VarId, 2> inputs = {engine->makeIntVar(0, lb, ub),
                                             engine->makeIntVar(1, lb, ub)};
  const VarId violationId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  BoolLessEqual& invariant = engine->makeConstraint<BoolLessEqual>(
      violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
      const VarId varId = invariant.nextInput(ts, *engine);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(BoolLessEqualTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<const VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId violationId = engine->makeIntVar(0, 0, ub - lb);
  BoolLessEqual& invariant = engine->makeConstraint<BoolLessEqual>(
      violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, valueDist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts, *engine);
      EXPECT_EQ(engine->value(ts, violationId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(BoolLessEqualTest, Commit) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::shuffle(indices.begin(), indices.end(), rng);

  engine->open();
  const std::array<const VarId, 2> inputs{
      engine->makeIntVar(committedValues.at(0), lb, ub),
      engine->makeIntVar(committedValues.at(1), lb, ub)};

  const VarId violationId = engine->makeIntVar(0, 0, 2);
  BoolLessEqual& invariant = engine->makeConstraint<BoolLessEqual>(
      violationId, inputs.at(0), inputs.at(1));
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), violationId),
            computeViolation(engine->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputs.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(engine->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      engine->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == engine->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, *engine, LocalId(i));

    // incremental value
    const Int notifiedViolation = engine->value(ts, violationId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedViolation, engine->value(ts, violationId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, violationId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedViolation, engine->value(ts + 1, violationId));
  }
}

class MockBoolLessEqual : public BoolLessEqual {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    BoolLessEqual::registerVars(engine);
  }
  explicit MockBoolLessEqual(VarId violationId, VarId x, VarId y)
      : BoolLessEqual(violationId, x, y) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return BoolLessEqual::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return BoolLessEqual::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          BoolLessEqual::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          BoolLessEqual::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      BoolLessEqual::commit(t, engine);
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
TEST_F(BoolLessEqualTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(5, 0, 100);
    const VarId y = engine->makeIntVar(0, 0, 100);
    const VarId viol = engine->makeIntVar(0, 0, 200);
    testNotifications<MockBoolLessEqual>(
        &engine->makeConstraint<MockBoolLessEqual>(viol, x, y), propMode,
        markingMode, 3, x, 1, viol);
  }
}
}  // namespace
