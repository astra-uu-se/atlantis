#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/notEqualConst.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class NotEqualConstTest : public InvariantTest {
 public:
  Int computeViolation(Timestamp ts, const VarId x, const Int y) {
    return computeViolation(engine->value(ts, x), y);
  }
  Int computeViolation(const Int xVal, const Int y) {
    return xVal == y ? 1 : 0;
  }
  bool insideBounds(const VarId x, const Int y) {
    return insideBounds(engine->lowerBound(x), engine->upperBound(x), y);
  }
  bool insideBounds(const Int lb, const Int ub, const Int y) {
    return lb <= y && y <= ub;
  }
  bool allEqual(const Int a, const Int b, const Int c) {
    return a == b && a == c;
  }
};

TEST_F(NotEqualConstTest, UpdateBounds) {
  const Int y = 0;

  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, -10}, {-5, 0}, {-2, 2}, {0, 5}, {10, 10}, {15, 20}};
  engine->open();
  const VarId x = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId violationId = engine->makeIntVar(0, 0, 1);
  NotEqualConst& invariant =
      engine->makeConstraint<NotEqualConst>(violationId, x, y);
  engine->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    engine->updateBounds(x, xLb, xUb, false);
    invariant.updateBounds(*engine);
    const Int lb = allEqual(xLb, xUb, y) ? 1 : 0;
    const Int ub = insideBounds(xLb, xUb, y) ? 1 : 0;

    ASSERT_EQ(engine->lowerBound(violationId), lb);
    ASSERT_EQ(engine->upperBound(violationId), ub);

    for (Int xVal = xLb; xVal <= xUb; ++xVal) {
      engine->setValue(engine->currentTimestamp(), x, xVal);
      invariant.updateBounds(*engine);
      invariant.recompute(engine->currentTimestamp(), *engine);
      ASSERT_GE(engine->value(engine->currentTimestamp(), violationId), lb);
      ASSERT_LE(engine->value(engine->currentTimestamp(), violationId), ub);
    }
  }
}

TEST_F(NotEqualConstTest, Recompute) {
  const Int xLb = -5;
  const Int xUb = 2;
  const Int yLb = -2;
  const Int yUb = 5;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  for (Int y = yLb; y <= yUb; ++y) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(xUb, xLb, xUb);
    const VarId violationId = engine->makeIntVar(0, 0, 1);
    NotEqualConst& invariant =
        engine->makeConstraint<NotEqualConst>(violationId, x, y);
    engine->close();

    for (Int xVal = xLb; xVal <= xUb; ++xVal) {
      engine->setValue(engine->currentTimestamp(), x, xVal);
      const Int expectedViolation =
          computeViolation(engine->currentTimestamp(), x, y);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(NotEqualConstTest, NotifyInputChanged) {
  const Int xLb = -5;
  const Int xUb = 2;
  const Int yLb = -2;
  const Int yUb = 5;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  for (Int y = yLb; y <= yUb; ++y) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(xUb, xLb, xUb);
    const VarId violationId = engine->makeIntVar(0, 0, 1);
    NotEqualConst& invariant =
        engine->makeConstraint<NotEqualConst>(violationId, x, y);
    engine->close();

    for (Int xVal = xLb; xVal <= xUb; ++xVal) {
      engine->setValue(engine->currentTimestamp(), x, xVal);
      const Int expectedViolation =
          computeViolation(engine->currentTimestamp(), x, y);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(0));
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(NotEqualConstTest, NextInput) {
  const Int lb = -5;
  const Int ub = 5;
  const Int y = 3;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const VarId x = engine->makeIntVar(lb, lb, ub);
  const VarId violationId = engine->makeIntVar(0, 0, 2);
  NotEqualConst& invariant =
      engine->makeConstraint<NotEqualConst>(violationId, x, y);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    EXPECT_EQ(invariant.nextInput(ts, *engine), x);
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
  }
}

TEST_F(NotEqualConstTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  const Int y = 4;
  std::uniform_int_distribution<Int> valueDist(lb, ub);

  engine->open();
  const VarId x = engine->makeIntVar(lb, lb, ub);
  const VarId violationId = engine->makeIntVar(0, 0, 2);
  NotEqualConst& invariant =
      engine->makeConstraint<NotEqualConst>(violationId, x, y);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    EXPECT_EQ(invariant.nextInput(ts, *engine), x);
    const Int oldVal = engine->value(ts, x);
    do {
      engine->setValue(ts, x, valueDist(gen));
    } while (engine->value(ts, x) == oldVal);
    invariant.notifyCurrentInputChanged(ts, *engine);
    EXPECT_EQ(engine->value(ts, violationId), computeViolation(ts, x, y));
  }
}

TEST_F(NotEqualConstTest, Commit) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);
  const Int y = 5;

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  Int committedValue = valueDist(gen);

  const VarId x = engine->makeIntVar(committedValue, lb, ub);

  const VarId violationId = engine->makeIntVar(0, 0, 1);
  NotEqualConst& invariant =
      engine->makeConstraint<NotEqualConst>(violationId, x, y);
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), violationId),
            computeViolation(engine->currentTimestamp(), x, y));

  Timestamp ts = engine->currentTimestamp() + Timestamp(1);
  // Check that we do not accidentally commit:
  ASSERT_EQ(engine->committedValue(x), committedValue);

  const Int oldVal = committedValue;
  do {
    engine->setValue(ts, x, valueDist(gen));
  } while (oldVal == engine->value(ts, x));

  // notify changes
  invariant.notifyInputChanged(ts, *engine, LocalId(0));

  // incremental value
  const Int notifiedViolation = engine->value(ts, violationId);
  invariant.recompute(ts, *engine);

  ASSERT_EQ(notifiedViolation, engine->value(ts, violationId));

  engine->commitIf(ts, x);
  committedValue = engine->value(ts, x);
  engine->commitIf(ts, violationId);

  invariant.commit(ts, *engine);
  invariant.recompute(ts + 1, *engine);
  ASSERT_EQ(notifiedViolation, engine->value(ts + 1, violationId));
}

class MockPowDomain : public NotEqualConst {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    NotEqualConst::registerVars(engine);
  }
  MockPowDomain(VarId violationId, VarId x, Int y)
      : NotEqualConst(violationId, x, y) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return NotEqualConst::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return NotEqualConst::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          NotEqualConst::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          NotEqualConst::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      NotEqualConst::commit(t, engine);
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
TEST_F(NotEqualConstTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(5, -100, 100);
    const Int y = 0;
    const VarId viol = engine->makeIntVar(0, 0, 1);
    testNotifications<MockPowDomain>(
        &engine->makeInvariant<MockPowDomain>(viol, x, y), propMode,
        markingMode, 2, x, 0, viol);
  }
}

}  // namespace
