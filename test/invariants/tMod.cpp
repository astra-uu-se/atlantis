#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/mod.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class ModTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId a, const VarId b) {
    Int denominator = engine->value(ts, b);
    if (denominator == 0) {
      denominator = engine->upperBound(b) > 0 ? 1 : -1;
    }
    return engine->value(ts, a) % std::abs(denominator);
  }
};

TEST_F(ModTest, Examples) {
  std::vector<std::array<Int, 3>> data{
      {7, 4, 3}, {-7, 4, -3}, {7, -4, 3}, {-7, -4, -3}};

  Int aLb = std::numeric_limits<Int>::max();
  Int aUb = std::numeric_limits<Int>::min();
  Int bLb = std::numeric_limits<Int>::max();
  Int bUb = std::numeric_limits<Int>::min();
  Int outputLb = std::numeric_limits<Int>::max();
  Int outputUb = std::numeric_limits<Int>::min();

  for (const auto& [aVal, bVal, outputVal] : data) {
    aLb = std::min(aLb, aVal);
    aUb = std::max(aUb, aVal);
    bLb = std::min(bLb, bVal);
    bUb = std::max(bUb, bVal);
    outputLb = std::min(outputLb, outputVal);
    outputUb = std::max(outputUb, outputVal);
  }
  EXPECT_TRUE(aLb <= aUb);
  EXPECT_TRUE(bLb <= bUb);
  EXPECT_TRUE(bLb != 0 || bUb != 0);

  engine->open();
  const VarId a = engine->makeIntVar(aUb, aLb, aUb);
  const VarId b = engine->makeIntVar(bUb, bLb, bUb);
  const VarId outputId = engine->makeIntVar(0, outputLb, outputUb);
  Mod& invariant = engine->makeInvariant<Mod>(a, b, outputId);
  engine->close();

  for (size_t i = 0; i < data.size(); ++i) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(i + 1);
    const Int aVal = data.at(i).at(0);
    const Int bVal = data.at(i).at(1);
    const Int expectedOutput = data.at(i).at(2);

    engine->setValue(ts, a, aVal);
    engine->setValue(ts, b, bVal);

    invariant.recompute(ts, *engine);

    EXPECT_EQ(engine->value(ts, outputId), expectedOutput);
    EXPECT_EQ(computeOutput(ts, a, b), expectedOutput);
  }
}

TEST_F(ModTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  engine->open();
  const VarId a = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId b = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  Mod& invariant = engine->makeInvariant<Mod>(a, b, outputId);
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

TEST_F(ModTest, Recompute) {
  const Int aLb = -1;
  const Int aUb = 0;
  const Int bLb = 0;
  const Int bUb = 1;
  const Int outputLb = -1;
  const Int outputUb = 0;

  EXPECT_TRUE(aLb <= aUb);
  EXPECT_TRUE(bLb <= bUb);
  EXPECT_TRUE(bLb != 0 || bUb != 0);

  engine->open();
  const VarId a = engine->makeIntVar(aUb, aLb, aUb);
  const VarId b = engine->makeIntVar(bUb, bLb, bUb);
  const VarId outputId = engine->makeIntVar(outputLb, outputLb, outputUb);
  Mod& invariant = engine->makeInvariant<Mod>(a, b, outputId);
  engine->close();

  for (Int aVal = aLb; aVal <= aUb; ++aVal) {
    for (Int bVal = bLb; bVal <= bUb; ++bVal) {
      engine->setValue(engine->currentTimestamp(), a, aVal);
      engine->setValue(engine->currentTimestamp(), b, bVal);

      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), a, b);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::array<VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                              engine->makeIntVar(ub, lb, ub)};
  VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  Mod& invariant =
      engine->makeInvariant<Mod>(inputs.at(0), inputs.at(1), outputId);
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

TEST_F(ModTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  const std::array<VarId, 2> inputs = {engine->makeIntVar(lb, lb, ub),
                                       engine->makeIntVar(ub, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  ;
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  ;
  Mod& invariant =
      engine->makeInvariant<Mod>(inputs.at(0), inputs.at(1), outputId);
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

TEST_F(ModTest, NotifyCurrentInputChanged) {
  const Int lb = -10002;
  const Int ub = -10000;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  Mod& invariant =
      engine->makeInvariant<Mod>(inputs.at(0), inputs.at(1), outputId);
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

TEST_F(ModTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      engine->makeIntVar(committedValues.at(0), lb, ub),
      engine->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = engine->makeIntVar(0, 0, 2);
  Mod& invariant =
      engine->makeInvariant<Mod>(inputs.at(0), inputs.at(1), outputId);
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

TEST_F(ModTest, ZeroDenominator) {
  const Int aVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [bLb, bUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, 0}, {-50, 50, 0}, {0, 100, 0}}) {
    EXPECT_TRUE(bLb <= bUb);
    EXPECT_TRUE(bLb != 0 || bUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      engine->open();
      const VarId a = engine->makeIntVar(aVal, aVal, aVal);
      const VarId b = engine->makeIntVar(0, bLb, bUb);
      const VarId outputId = engine->makeIntVar(0, outputLb, outputUb);
      Mod& invariant = engine->makeInvariant<Mod>(a, b, outputId);
      engine->close();

      EXPECT_EQ(expected, computeOutput(engine->currentTimestamp(), a, b));
      if (method == 0) {
        invariant.recompute(engine->currentTimestamp(), *engine);
      } else {
        invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                     LocalId(1));
      }
      EXPECT_EQ(expected, engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

class MockMod : public Mod {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    Mod::registerVars(engine);
  }
  MockMod(VarId a, VarId b, VarId c) : Mod(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Mod::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return Mod::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          Mod::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          Mod::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      Mod::commit(t, engine);
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
TEST_F(ModTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId a = engine->makeIntVar(-10, -100, 100);
    const VarId b = engine->makeIntVar(10, -100, 100);
    const VarId output = engine->makeIntVar(0, 0, 200);
    testNotifications<MockMod>(&engine->makeInvariant<MockMod>(a, b, output),
                               propMode, markingMode, 3, a, 0, output);
  }
}

}  // namespace
