#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/intDiv.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class IntDivTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId x, const VarId y) {
    Int denominator = engine->value(ts, y);
    if (denominator == 0) {
      denominator = engine->upperBound(y) > 0 ? 1 : -1;
    }
    return engine->value(ts, x) / denominator;
  }
};

TEST_F(IntDivTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  engine->open();
  const VarId x = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  IntDiv& invariant = engine->makeInvariant<IntDiv>(*engine, outputId, x, y);
  engine->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    engine->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      engine->updateBounds(y, yLb, yUb, false);
      engine->open();
      invariant.updateBounds();
      engine->close();
      std::vector<Int> outputs;
      const Int lb = engine->lowerBound(outputId);
      const Int ub = engine->upperBound(outputId);
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        engine->setValue(engine->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          engine->setValue(engine->currentTimestamp(), y, yVal);
          invariant.recompute(engine->currentTimestamp());
          const Int o = engine->value(engine->currentTimestamp(), outputId);
          if (o < lb || ub < o) {
            ASSERT_TRUE(lb <= o);
            ASSERT_TRUE(o <= ub);
          }
          outputs.emplace_back(o);
        }
      }
      const auto& [minVal, maxVal] =
          std::minmax_element(outputs.begin(), outputs.end());
      if (*minVal != engine->lowerBound(outputId)) {
        ASSERT_EQ(*minVal, engine->lowerBound(outputId));
      }
      ASSERT_EQ(*maxVal, engine->upperBound(outputId));
    }
  }
}

TEST_F(IntDivTest, Recompute) {
  const Int xLb = -1;
  const Int xUb = 0;
  const Int yLb = 0;
  const Int yUb = 1;
  const Int outputLb = -1;
  const Int outputUb = 0;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  EXPECT_TRUE(yLb != 0 || yUb != 0);

  engine->open();
  const VarId x = engine->makeIntVar(xUb, xLb, xUb);
  const VarId y = engine->makeIntVar(yUb, yLb, yUb);
  const VarId outputId = engine->makeIntVar(0, outputLb, outputUb);
  IntDiv& invariant = engine->makeInvariant<IntDiv>(*engine, outputId, x, y);
  engine->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      engine->setValue(engine->currentTimestamp(), x, xVal);
      engine->setValue(engine->currentTimestamp(), y, yVal);

      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), x, y);
      invariant.recompute(engine->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(IntDivTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::array<VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                              engine->makeIntVar(ub, lb, ub)};
  VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  IntDiv& invariant = engine->makeInvariant<IntDiv>(*engine, outputId,
                                                    inputs.at(0), inputs.at(1));
  engine->close();

  Timestamp ts = engine->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(ts, inputs.at(i), val);
      const Int expectedOutput = computeOutput(ts, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, engine->value(ts, outputId));
    }
  }
}

TEST_F(IntDivTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  const std::array<VarId, 2> inputs = {engine->makeIntVar(lb, lb, ub),
                                       engine->makeIntVar(ub, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  IntDiv& invariant = engine->makeInvariant<IntDiv>(*engine, outputId,
                                                    inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
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

TEST_F(IntDivTest, NotifyCurrentInputChanged) {
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
  IntDiv& invariant = engine->makeInvariant<IntDiv>(*engine, outputId,
                                                    inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, valueDist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(IntDivTest, Commit) {
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
  IntDiv& invariant = engine->makeInvariant<IntDiv>(*engine, outputId,
                                                    inputs.at(0), inputs.at(1));
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
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

TEST_F(IntDivTest, ZeroDenominator) {
  const Int xVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [yLb, yUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, -10}, {-50, 50, 10}, {0, 100, 10}}) {
    EXPECT_TRUE(yLb <= yUb);
    EXPECT_TRUE(yLb != 0 || yUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      engine->open();
      const VarId x = engine->makeIntVar(xVal, xVal, xVal);
      const VarId y = engine->makeIntVar(0, yLb, yUb);
      const VarId outputId = engine->makeIntVar(0, outputLb, outputUb);
      IntDiv& invariant =
          engine->makeInvariant<IntDiv>(*engine, outputId, x, y);
      engine->close();

      EXPECT_EQ(expected, computeOutput(engine->currentTimestamp(), x, y));
      if (method == 0) {
        invariant.recompute(engine->currentTimestamp());
      } else {
        invariant.notifyInputChanged(engine->currentTimestamp(), LocalId(1));
      }
      EXPECT_EQ(expected, engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

class MockIntDiv : public IntDiv {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    IntDiv::registerVars();
  }
  explicit MockIntDiv(Engine& engine, VarId x, VarId y, VarId c)
      : IntDiv(engine, x, y, c) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return IntDiv::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return IntDiv::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          IntDiv::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          IntDiv::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      IntDiv::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(IntDivTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(-10, -100, 100);
    const VarId y = engine->makeIntVar(10, -100, 100);
    const VarId output = engine->makeIntVar(0, 0, 200);
    testNotifications<MockIntDiv>(
        &engine->makeInvariant<MockIntDiv>(*engine, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace
