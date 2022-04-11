#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/times.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class TimesTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(engine->value(ts, inputs.at(0)),
                         engine->value(ts, inputs.at(1)));
  }

  Int computeOutput(std::array<Int, 2> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId x, const VarId y) {
    return computeOutput(engine->value(ts, x), engine->value(ts, y));
  }

  Int computeOutput(const Int aVal, const Int bVal) { return aVal * bVal; }
};

TEST_F(TimesTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  engine->open();
  const VarId a = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId b = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  Times& invariant = engine->makeInvariant<Times>(a, b, outputId);
  engine->close();

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_TRUE(aLb <= aUb);
    engine->updateBounds(a, aLb, aUb);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_TRUE(bLb <= bUb);
      engine->updateBounds(b, bLb, bUb);
      engine->open();
      invariant.updateBounds(*engine);
      engine->close();
      for (Int aVal = aLb; aVal <= aUb; ++aVal) {
        engine->setValue(engine->currentTimestamp(), a, aVal);
        for (Int bVal = bLb; bVal <= bUb; ++bVal) {
          engine->setValue(engine->currentTimestamp(), b, bVal);
          invariant.recompute(engine->currentTimestamp(), *engine);
          const Int o = engine->value(engine->currentTimestamp(), outputId);
          if (o < engine->lowerBound(outputId) ||
              engine->upperBound(outputId) < o) {
            ASSERT_GE(o, engine->lowerBound(outputId));
            ASSERT_LE(o, engine->upperBound(outputId));
          }
        }
      }
    }
  }
}

TEST_F(TimesTest, Recompute) {
  const Int aLb = 0;
  const Int aUb = 10;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(aLb <= aUb);
  EXPECT_TRUE(bLb <= bUb);

  engine->open();
  const VarId x = engine->makeIntVar(aUb, aLb, aUb);
  const VarId y = engine->makeIntVar(bUb, bLb, bUb);
  const VarId outputId =
      engine->makeIntVar(0, 0, std::max(aUb - bLb, bUb - aLb));
  Times& invariant = engine->makeInvariant<Times>(x, y, outputId);
  engine->close();

  for (Int aVal = aLb; aVal <= aUb; ++aVal) {
    for (Int bVal = bLb; bVal <= bUb; ++bVal) {
      engine->setValue(engine->currentTimestamp(), x, aVal);
      engine->setValue(engine->currentTimestamp(), y, bVal);

      const Int expectedOutput = computeOutput(aVal, bVal);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(TimesTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::array<VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                              engine->makeIntVar(ub, lb, ub)};
  VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  Times& invariant =
      engine->makeInvariant<Times>(inputs.at(0), inputs.at(1), outputId);
  engine->close();

  for (Int val = lb; val <= ub; ++val) {
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(engine->currentTimestamp(), inputs.at(i), val);
      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(i));
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(TimesTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<VarId, 2> inputs = {engine->makeIntVar(lb, lb, ub),
                                       engine->makeIntVar(ub, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  ;
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  ;
  Times& invariant =
      engine->makeInvariant<Times>(inputs.at(0), inputs.at(1), outputId);
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

TEST_F(TimesTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  Times& invariant =
      engine->makeInvariant<Times>(inputs.at(0), inputs.at(1), outputId);
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
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(TimesTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      engine->makeIntVar(committedValues.at(0), lb, ub),
      engine->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = engine->makeIntVar(0, 0, 2);
  Times& invariant =
      engine->makeInvariant<Times>(inputs.at(0), inputs.at(1), outputId);
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
            computeOutput(engine->currentTimestamp(), inputs));

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
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

class MockTimes : public Times {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    Times::registerVars(engine);
  }
  MockTimes(VarId a, VarId b, VarId c) : Times(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Times::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return Times::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          Times::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          Times::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      Times::commit(t, engine);
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
TEST_F(TimesTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId a = engine->makeIntVar(-10, -100, 100);
    const VarId b = engine->makeIntVar(10, -100, 100);
    const VarId output = engine->makeIntVar(0, 0, 200);
    testNotifications<MockTimes>(
        &engine->makeInvariant<MockTimes>(a, b, output), propMode, markingMode,
        3, a, 0, output);
  }
}

}  // namespace
