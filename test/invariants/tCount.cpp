#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/count.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class CountTest : public InvariantTest {
 public:
  Int computeOutput(const Timestamp ts, const VarId y,
                    const std::vector<VarId>& variables) {
    std::vector<Int> values(variables.size(), 0);
    for (size_t i = 0; i < variables.size(); ++i) {
      values.at(i) = engine->value(ts, variables.at(i));
    }
    return computeOutput(engine->value(ts, y), values);
  }

  Int computeOutput(const Int y, const std::vector<Int>& values) {
    Int count = 0;
    for (size_t i = 0; i < values.size(); ++i) {
      if (values.at(i) == y) {
        ++count;
      }
    }
    return count;
  }
};

TEST_F(CountTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, 0}, {-5, 5}, {0, 10}, {15, 20}};
  engine->open();

  const VarId y = engine->makeIntVar(0, 0, 10);
  std::vector<VarId> vars{engine->makeIntVar(0, 0, 10),
                          engine->makeIntVar(0, 0, 10),
                          engine->makeIntVar(0, 0, 10)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  Count& invariant = engine->makeInvariant<Count>(outputId, y, vars);

  for (const auto& [yLb, yUb] : boundVec) {
    EXPECT_TRUE(yLb <= yUb);
    engine->updateBounds(y, yLb, yUb, false);
    for (const auto& [aLb, aUb] : boundVec) {
      EXPECT_TRUE(aLb <= aUb);
      engine->updateBounds(vars.at(0), aLb, aUb, false);
      for (const auto& [bLb, bUb] : boundVec) {
        EXPECT_TRUE(bLb <= bUb);
        engine->updateBounds(vars.at(1), bLb, bUb, false);
        for (const auto& [cLb, cUb] : boundVec) {
          EXPECT_TRUE(cLb <= cUb);
          engine->updateBounds(vars.at(2), cLb, cUb, false);
          invariant.updateBounds(*engine);

          ASSERT_GE(0, engine->lowerBound(outputId));
          ASSERT_LE(vars.size(), engine->upperBound(outputId));
        }
      }
    }
  }
}

TEST_F(CountTest, Recompute) {
  const Int lb = -5;
  const Int ub = 5;

  ASSERT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> dist(lb, ub);

  engine->open();

  const VarId y = engine->makeIntVar(dist(gen), lb, ub);
  const VarId a = engine->makeIntVar(dist(gen), lb, ub);
  const VarId b = engine->makeIntVar(dist(gen), lb, ub);
  const VarId c = engine->makeIntVar(dist(gen), lb, ub);

  std::vector<VarId> inputs{a, b, c};

  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());

  Count& invariant = engine->makeInvariant<Count>(outputId, y, inputs);
  engine->close();

  for (Int yVal = lb; yVal <= ub; ++yVal) {
    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          engine->setValue(engine->currentTimestamp(), y, yVal);
          engine->setValue(engine->currentTimestamp(), a, aVal);
          engine->setValue(engine->currentTimestamp(), b, bVal);
          engine->setValue(engine->currentTimestamp(), c, cVal);
          const Int expectedOutput =
              computeOutput(engine->currentTimestamp(), y, inputs);
          invariant.recompute(engine->currentTimestamp(), *engine);
          EXPECT_EQ(expectedOutput,
                    engine->value(engine->currentTimestamp(), outputId));
        }
      }
    }
  }
}

TEST_F(CountTest, NotifyInputChanged) {
  engine->open();
  const size_t numInputs = 3;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  const VarId y = engine->makeIntVar(dist(gen), lb, ub);
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Count& invariant = engine->makeInvariant<Count>(outputId, y, inputs);
  engine->close();

  std::vector<VarId> allInputs(inputs);
  allInputs.emplace_back(y);

  for (size_t i = 0; i < allInputs.size(); ++i) {
    const Int oldVal =
        engine->value(engine->currentTimestamp(), allInputs.at(i));
    do {
      engine->setValue(engine->currentTimestamp(), allInputs.at(i), dist(gen));
    } while (oldVal ==
             engine->value(engine->currentTimestamp(), allInputs.at(i)));

    const Int expectedOutput =
        computeOutput(engine->currentTimestamp(), y, inputs);

    invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                 LocalId(i));
    EXPECT_EQ(expectedOutput,
              engine->value(engine->currentTimestamp(), outputId));
  }
}

TEST_F(CountTest, NextInput) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);

  engine->open();
  const VarId y = engine->makeIntVar(dist(gen), lb, ub);
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Count& invariant = engine->makeInvariant<Count>(outputId, y, inputs);
  engine->close();

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId minVarId =
      std::min(y, *std::min_element(inputs.begin(), inputs.end()));
  const VarId maxVarId =
      std::max(y, *std::max_element(inputs.begin(), inputs.end()));

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i <= numInputs; ++i) {
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

TEST_F(CountTest, NotifyCurrentInputChanged) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  engine->open();
  const VarId y = engine->makeIntVar(dist(gen), lb, ub);
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
  }

  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Count& invariant = engine->makeInvariant<Count>(outputId, y, inputs);
  engine->close();

  std::vector<VarId> allInputs(inputs);
  allInputs.emplace_back(y);

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : allInputs) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, dist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts, *engine);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, y, inputs));
    }
  }
}

TEST_F(CountTest, Commit) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);
  std::vector<VarId> inputs(numInputs, NULL_ID);
  std::vector<size_t> indices(numInputs + 1, 0);
  std::vector<Int> committedValues(numInputs + 1, 0);

  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    committedValues.at(i) = dist(gen);
    inputs.at(i) = engine->makeIntVar(committedValues.at(i), lb, ub);
  }
  indices.back() = numInputs;
  committedValues.back() = dist(gen);
  const VarId y = engine->makeIntVar(committedValues.back(), lb, ub);

  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Count& invariant = engine->makeInvariant<Count>(outputId, y, inputs);

  std::shuffle(indices.begin(), indices.end(), rng);

  engine->close();
  std::vector<VarId> allInputs(inputs);
  allInputs.emplace_back(y);

  EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
            computeOutput(engine->currentTimestamp(), y, inputs));

  for (const size_t i : indices) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(engine->committedValue(allInputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      engine->setValue(ts, allInputs.at(i), dist(gen));
    } while (oldVal == engine->value(ts, allInputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, *engine, LocalId(i));

    // incremental value
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, allInputs.at(i));
    committedValues.at(i) = engine->value(ts, allInputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

class MockCount : public Count {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    Count::registerVars(engine);
  }
  explicit MockCount(VarId output, VarId y, std::vector<VarId> varArray)
      : Count(output, y, varArray) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Count::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return Count::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          Count::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          Count::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      Count::commit(t, engine);
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
TEST_F(CountTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const Int numArgs = 10;
    const VarId y = engine->makeIntVar(0, 0, numArgs);
    std::vector<VarId> args;
    for (Int value = 1; value <= numArgs; ++value) {
      args.push_back(engine->makeIntVar(value, 1, numArgs));
    }
    const VarId modifiedVarId = args.front();
    const VarId output = engine->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCount>(
        &engine->makeInvariant<MockCount>(output, y, args), propMode,
        markingMode, numArgs + 2, modifiedVarId, 5, output);
  }
}

}  // namespace
