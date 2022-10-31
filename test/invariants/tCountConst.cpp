#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/countConst.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class CountConstTest : public InvariantTest {
 public:
  Int computeOutput(const Timestamp ts, const Int y,
                    const std::vector<VarId>& variables) {
    std::vector<Int> values(variables.size(), 0);
    for (size_t i = 0; i < variables.size(); ++i) {
      values.at(i) = engine->value(ts, variables.at(i));
    }
    return computeOutput(y, values);
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

TEST_F(CountConstTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, 0}, {-5, 5}, {0, 10}, {15, 20}};
  engine->open();

  const Int y = 10;
  std::vector<VarId> inputs{engine->makeIntVar(0, 0, 10),
                            engine->makeIntVar(0, 0, 10),
                            engine->makeIntVar(0, 0, 10)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  CountConst& invariant =
      engine->makeInvariant<CountConst>(*engine, outputId, y, inputs);

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_TRUE(aLb <= aUb);
    engine->updateBounds(inputs.at(0), aLb, aUb, false);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_TRUE(bLb <= bUb);
      engine->updateBounds(inputs.at(1), bLb, bUb, false);
      for (const auto& [cLb, cUb] : boundVec) {
        EXPECT_TRUE(cLb <= cUb);
        engine->updateBounds(inputs.at(2), cLb, cUb, false);
        invariant.updateBounds();

        ASSERT_GE(0, engine->lowerBound(outputId));
        ASSERT_LE(inputs.size(), engine->upperBound(outputId));
      }
    }
  }
}

TEST_F(CountConstTest, Recompute) {
  const Int lb = -5;
  const Int ub = 5;

  ASSERT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> dist(lb, ub);

  for (Int y = lb; y <= ub; ++y) {
    engine->open();

    const VarId a = engine->makeIntVar(dist(gen), lb, ub);
    const VarId b = engine->makeIntVar(dist(gen), lb, ub);
    const VarId c = engine->makeIntVar(dist(gen), lb, ub);

    std::vector<VarId> inputs{a, b, c};

    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

    CountConst& invariant =
        engine->makeInvariant<CountConst>(*engine, outputId, y, inputs);
    engine->close();

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          engine->setValue(engine->currentTimestamp(), a, aVal);
          engine->setValue(engine->currentTimestamp(), b, bVal);
          engine->setValue(engine->currentTimestamp(), c, cVal);
          const Int expectedOutput =
              computeOutput(engine->currentTimestamp(), y, inputs);
          invariant.recompute(engine->currentTimestamp());
          EXPECT_EQ(expectedOutput,
                    engine->value(engine->currentTimestamp(), outputId));
        }
      }
    }
  }
}

TEST_F(CountConstTest, NotifyInputChanged) {
  const size_t numInputs = 3;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  for (Int y = lb; y <= ub; ++y) {
    engine->open();
    std::vector<VarId> inputs(numInputs, NULL_ID);
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
    }
    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant =
        engine->makeInvariant<CountConst>(*engine, outputId, y, inputs);
    engine->close();

    for (size_t i = 0; i < inputs.size(); ++i) {
      const Int oldVal =
          engine->value(engine->currentTimestamp(), inputs.at(i));
      do {
        engine->setValue(engine->currentTimestamp(), inputs.at(i), dist(gen));
      } while (oldVal ==
               engine->value(engine->currentTimestamp(), inputs.at(i)));

      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), y, inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), LocalId(i));
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(CountConstTest, NextInput) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  const Int y = 0;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);

  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  CountConst& invariant =
      engine->makeInvariant<CountConst>(*engine, outputId, y, inputs);
  engine->close();

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
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

TEST_F(CountConstTest, NotifyCurrentInputChanged) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
  }

  for (Int y = lb; y <= ub; ++y) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant =
        engine->makeInvariant<CountConst>(*engine, outputId, y, inputs);
    engine->close();

    for (Timestamp ts = engine->currentTimestamp() + 1;
         ts < engine->currentTimestamp() + 4; ++ts) {
      for (const VarId varId : inputs) {
        EXPECT_EQ(invariant.nextInput(ts), varId);
        const Int oldVal = engine->value(ts, varId);
        do {
          engine->setValue(ts, varId, dist(gen));
        } while (engine->value(ts, varId) == oldVal);
        invariant.notifyCurrentInputChanged(ts);
        EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, y, inputs));
      }
    }
  }
}

TEST_F(CountConstTest, Commit) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  std::vector<size_t> indices(numInputs, 0);
  std::vector<Int> committedValues(numInputs, 0);

  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    inputs.at(i) = engine->makeIntVar(dist(gen), lb, ub);
  }

  for (Int y = lb; y <= ub; ++y) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId outputId = engine->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant =
        engine->makeInvariant<CountConst>(*engine, outputId, y, inputs);

    engine->close();

    for (size_t i = 0; i < numInputs; ++i) {
      committedValues.at(i) = engine->committedValue(inputs.at(i));
    }

    std::shuffle(indices.begin(), indices.end(), rng);

    EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
              computeOutput(engine->currentTimestamp(), y, inputs));

    for (const size_t i : indices) {
      Timestamp ts = engine->currentTimestamp() + Timestamp(i);
      for (size_t j = 0; j < numInputs; ++j) {
        // Check that we do not accidentally commit:
        ASSERT_EQ(engine->committedValue(inputs.at(j)), committedValues.at(j));
      }

      const Int oldVal = committedValues.at(i);
      do {
        engine->setValue(ts, inputs.at(i), dist(gen));
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
}

class MockCountConst : public CountConst {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    CountConst::registerVars();
  }
  explicit MockCountConst(Engine& engine, VarId output, Int y,
                          const std::vector<VarId>& varArray)
      : CountConst(engine, output, y, varArray) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return CountConst::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return CountConst::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          CountConst::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          CountConst::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      CountConst::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(CountConstTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const Int numArgs = 10;
    const Int y = 5;
    std::vector<VarId> varArray;
    for (Int value = 1; value <= numArgs; ++value) {
      varArray.push_back(engine->makeIntVar(value, 1, numArgs));
    }
    const VarId modifiedVarId = varArray.front();
    const VarId output = engine->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCountConst>(
        &engine->makeInvariant<MockCountConst>(*engine, output, y, varArray),
        propMode, markingMode, numArgs + 1, modifiedVarId, 5, output);
  }
}

}  // namespace
