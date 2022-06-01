#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/boolAnd.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class BoolAndTest : public InvariantTest {
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
    return std::max(xVal, yVal);
  }
};

TEST_F(BoolAndTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 10}, {1, 10}, {10, 100}};
  engine->open();
  const VarId x = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  BoolAnd& invariant = engine->makeInvariant<BoolAnd>(outputId, x, y);
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
      ASSERT_GE(std::max(xLb, yLb), engine->lowerBound(outputId));
      ASSERT_EQ(std::max(xUb, yUb), engine->upperBound(outputId));
    }
  }
}

TEST_F(BoolAndTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 100;
  const Int yLb = 0;
  const Int yUb = 100;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  engine->open();
  const std::array<const VarId, 2> inputs{engine->makeIntVar(xUb, xLb, xUb),
                                          engine->makeIntVar(yUb, yLb, yUb)};
  const VarId outputId =
      engine->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  BoolAnd& invariant =
      engine->makeInvariant<BoolAnd>(outputId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      engine->setValue(engine->currentTimestamp(), inputs.at(0), xVal);
      engine->setValue(engine->currentTimestamp(), inputs.at(1), yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(BoolAndTest, NotifyInputChanged) {
  const Int lb = 0;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<const VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                                          engine->makeIntVar(ub, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  BoolAnd& invariant =
      engine->makeInvariant<BoolAnd>(outputId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Int val = lb; val <= ub; ++val) {
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(engine->currentTimestamp(), inputs.at(i), val);
      const Int expectedViolation =
          computeViolation(engine->currentTimestamp(), inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(i));
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(BoolAndTest, NextInput) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<const VarId, 2> inputs = {engine->makeIntVar(0, lb, ub),
                                             engine->makeIntVar(1, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  BoolAnd& invariant =
      engine->makeInvariant<BoolAnd>(outputId, inputs.at(0), inputs.at(1));
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

TEST_F(BoolAndTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<const VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  BoolAnd& invariant =
      engine->makeInvariant<BoolAnd>(outputId, inputs.at(0), inputs.at(1));
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
      EXPECT_EQ(engine->value(ts, outputId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(BoolAndTest, Commit) {
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

  const VarId outputId = engine->makeIntVar(0, 0, 2);
  BoolAnd& invariant =
      engine->makeInvariant<BoolAnd>(outputId, inputs.at(0), inputs.at(1));
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
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
    const Int notifiedViolation = engine->value(ts, outputId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedViolation, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedViolation, engine->value(ts + 1, outputId));
  }
}

class MockBoolAnd : public BoolAnd {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    BoolAnd::registerVars(engine);
  }
  explicit MockBoolAnd(VarId outputId, VarId x, VarId y)
      : BoolAnd(outputId, x, y) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return BoolAnd::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return BoolAnd::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          BoolAnd::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          BoolAnd::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      BoolAnd::commit(t, engine);
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
TEST_F(BoolAndTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(5, 0, 100);
    const VarId y = engine->makeIntVar(0, 0, 100);
    const VarId output = engine->makeIntVar(0, 0, 200);
    testNotifications<MockBoolAnd>(
        &engine->makeInvariant<MockBoolAnd>(output, x, y), propMode,
        markingMode, 3, x, 1, output);
  }
}
}  // namespace
