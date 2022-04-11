
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/ifThenElse.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class IfThenElseTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 3> inputs) {
    return computeOutput(engine->value(ts, inputs.at(0)),
                         engine->value(ts, inputs.at(1)),
                         engine->value(ts, inputs.at(2)));
  }

  Int computeOutput(std::array<Int, 3> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1), inputs.at(2));
  }

  Int computeOutput(Timestamp ts, const VarId b, const VarId x, const VarId y) {
    return computeOutput(engine->value(ts, b), engine->value(ts, x),
                         engine->value(ts, y));
  }

  Int computeOutput(const Int bVal, const Int xVal, const Int yVal) {
    return bVal == 0 ? xVal : yVal;
  }
};

TEST_F(IfThenElseTest, UpdateBounds) {
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 100;
  const Int yUb = 1000;
  EXPECT_TRUE(xLb <= xUb);

  engine->open();
  const VarId b = engine->makeIntVar(0, 0, 10);
  const VarId x = engine->makeIntVar(yUb, yLb, yUb);
  const VarId y = engine->makeIntVar(yUb, yLb, yUb);
  const VarId outputId =
      engine->makeIntVar(0, std::min(xLb, yLb), std::max(xUb, yUb));
  IfThenElse& invariant = engine->makeInvariant<IfThenElse>(b, x, y, outputId);
  engine->close();

  std::vector<std::pair<Int, Int>> bBounds{{0, 0}, {0, 100}, {1, 10000}};

  for (const auto& [bLb, bUb] : bBounds) {
    EXPECT_TRUE(bLb <= bUb);
    engine->updateBounds(b, bLb, bUb);
    invariant.updateBounds(*engine);
    if (bLb == 0 && bUb == 0) {
      EXPECT_EQ(engine->lowerBound(outputId), engine->lowerBound(x));
      EXPECT_EQ(engine->upperBound(outputId), engine->upperBound(x));
    } else if (bLb > 0) {
      EXPECT_EQ(engine->lowerBound(outputId), engine->lowerBound(y));
      EXPECT_EQ(engine->upperBound(outputId), engine->upperBound(y));
    } else {
      EXPECT_EQ(engine->lowerBound(outputId),
                std::max(engine->lowerBound(x), engine->lowerBound(y)));
      EXPECT_EQ(engine->upperBound(outputId),
                std::min(engine->upperBound(x), engine->upperBound(y)));
    }
  }
}

TEST_F(IfThenElseTest, Recompute) {
  const Int bLb = 0;
  const Int bUb = 5;
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 0;
  const Int yUb = 5;
  EXPECT_TRUE(bLb <= bUb);
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  engine->open();
  const VarId b = engine->makeIntVar(bLb, bLb, bUb);
  const VarId x = engine->makeIntVar(yUb, yLb, yUb);
  const VarId y = engine->makeIntVar(yUb, yLb, yUb);
  const VarId outputId =
      engine->makeIntVar(0, std::min(xLb, yLb), std::max(xUb, yUb));
  IfThenElse& invariant = engine->makeInvariant<IfThenElse>(b, x, y, outputId);
  engine->close();
  for (Int bVal = bLb; bVal <= bUb; ++bVal) {
    for (Int xVal = xLb; xVal <= xUb; ++xVal) {
      for (Int yVal = yLb; yVal <= yUb; ++yVal) {
        engine->setValue(engine->currentTimestamp(), b, bVal);
        engine->setValue(engine->currentTimestamp(), x, xVal);
        engine->setValue(engine->currentTimestamp(), y, yVal);

        const Int expectedOutput = computeOutput(bVal, xVal, yVal);
        invariant.recompute(engine->currentTimestamp(), *engine);
        EXPECT_EQ(expectedOutput,
                  engine->value(engine->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(IfThenElseTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  engine->open();
  std::array<VarId, 3> inputs{engine->makeIntVar(bLb, bLb, bUb),
                              engine->makeIntVar(ub, lb, ub),
                              engine->makeIntVar(ub, lb, ub)};
  VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  IfThenElse& invariant = engine->makeInvariant<IfThenElse>(
      inputs.at(0), inputs.at(1), inputs.at(2), outputId);
  engine->close();

  for (Int bVal = bLb; bVal <= bUb; ++bVal) {
    for (Int val = lb; val <= ub; ++val) {
      for (size_t i = 1; i < inputs.size(); ++i) {
        engine->setValue(engine->currentTimestamp(), inputs.at(0), bVal);
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
}

TEST_F(IfThenElseTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  engine->open();
  const std::array<VarId, 3> inputs = {engine->makeIntVar(bLb, bLb, bUb),
                                       engine->makeIntVar(lb, lb, ub),
                                       engine->makeIntVar(ub, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  ;
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  ;
  IfThenElse& invariant = engine->makeInvariant<IfThenElse>(
      inputs.at(0), inputs.at(1), inputs.at(2), outputId);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    // First input is b,
    // Second input is x if b = 0, otherwise y:
    for (size_t i = 0; i < 2; ++i) {
      const VarId varId = invariant.nextInput(ts, *engine);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    const Int bVal = engine->value(ts, inputs.at(0));

    EXPECT_TRUE(notified.at(inputs.at(0)));
    EXPECT_TRUE(notified.at(inputs.at(bVal == 0 ? 1 : 2)));
    EXPECT_FALSE(notified.at(inputs.at(bVal == 0 ? 2 : 1)));
  }
}

TEST_F(IfThenElseTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<Int> bDist(bLb, bUb);

  const std::array<VarId, 3> inputs = {
      engine->makeIntVar(bLb, bLb, bLb),
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  IfThenElse& invariant = engine->makeInvariant<IfThenElse>(
      inputs.at(0), inputs.at(1), inputs.at(2), outputId);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (size_t i = 0; i < 2; ++i) {
      const Int bOld = engine->value(ts, inputs.at(0));
      const VarId curInput = invariant.nextInput(ts, *engine);
      EXPECT_EQ(curInput, inputs.at(i == 0 ? 0 : bOld == 0 ? 1 : 2));

      const Int oldVal = engine->value(ts, curInput);
      do {
        engine->setValue(ts, curInput, i == 0 ? bDist(gen) : valueDist(gen));
      } while (engine->value(ts, curInput) == oldVal);

      invariant.notifyCurrentInputChanged(ts, *engine);

      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(IfThenElseTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  engine->open();
  std::uniform_int_distribution<Int> bDist(bLb, bUb);
  std::uniform_int_distribution<Int> valueDist(lb, ub);

  std::array<size_t, 3> indices{0, 1, 2};
  std::array<Int, 3> committedValues{bDist(gen), valueDist(gen),
                                     valueDist(gen)};
  std::array<VarId, 3> inputs{
      engine->makeIntVar(committedValues.at(0), bLb, bUb),
      engine->makeIntVar(committedValues.at(1), lb, ub),
      engine->makeIntVar(committedValues.at(2), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = engine->makeIntVar(0, 0, 2);
  IfThenElse& invariant = engine->makeInvariant<IfThenElse>(
      inputs.at(0), inputs.at(1), inputs.at(2), outputId);
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
      engine->setValue(ts, inputs.at(i), i == 0 ? bDist(gen) : valueDist(gen));
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

class MockIfThenElse : public IfThenElse {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    IfThenElse::registerVars(engine);
  }
  MockIfThenElse(VarId b, VarId x, VarId y, VarId z) : IfThenElse(b, x, y, z) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return IfThenElse::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return IfThenElse::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          IfThenElse::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          IfThenElse::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      IfThenElse::commit(t, engine);
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
TEST_F(IfThenElseTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId b = engine->makeIntVar(0, -100, 100);
    const VarId x = engine->makeIntVar(0, 0, 4);
    const VarId y = engine->makeIntVar(5, 5, 9);
    const VarId output = engine->makeIntVar(3, 0, 9);
    testNotifications<MockIfThenElse>(
        &engine->makeInvariant<MockIfThenElse>(b, x, y, output), propMode,
        markingMode, 4, b, 5, output);
  }
}

}  // namespace
