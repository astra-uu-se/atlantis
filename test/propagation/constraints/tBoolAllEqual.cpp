#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/constraints/boolAllEqual.hpp"
#include "propagation/propagationEngine.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolAllEqualTest : public InvariantTest {
 public:
  bool isRegistered = false;

  Int computeViolation(Timestamp ts, const std::vector<VarId>& variables) {
    std::vector<Int> values(variables.size(), 0);
    for (size_t i = 0; i < variables.size(); ++i) {
      values.at(i) = engine->value(ts, variables.at(i));
    }
    return computeViolation(values);
  }

  Int computeViolation(const std::vector<Int>& values) {
    size_t numFalse = 0;
    size_t numTrue = 0;
    for (const Int val : values) {
      if (val == 0) {
        ++numTrue;
      } else {
        ++numFalse;
      }
    }
    return std::min(values.size() - numTrue, values.size() - numFalse);
  }
};

TEST_F(BoolAllEqualTest, UpdateBounds) {}

TEST_F(BoolAllEqualTest, Recompute) {
  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 5}, {10, 10}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_TRUE(lb <= ub);
    engine->open();
    const VarId a = engine->makeIntVar(lb, lb, ub);
    const VarId b = engine->makeIntVar(lb, lb, ub);
    const VarId c = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntVar(0, 0, 2);
    BoolAllEqual& invariant = engine->makeConstraint<BoolAllEqual>(
        *engine, violationId, std::vector<VarId>{a, b, c});
    engine->close();

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          engine->setValue(engine->currentTimestamp(), a, aVal);
          engine->setValue(engine->currentTimestamp(), b, bVal);
          engine->setValue(engine->currentTimestamp(), c, cVal);
          const Int expectedViolation =
              computeViolation(std::vector{aVal, bVal, cVal});
          invariant.recompute(engine->currentTimestamp());
          EXPECT_EQ(expectedViolation,
                    engine->value(engine->currentTimestamp(), violationId));
        }
      }
    }
  }
}

TEST_F(BoolAllEqualTest, NotifyInputChanged) {
  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 5}, {10, 10}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_TRUE(lb <= ub);

    engine->open();
    std::vector<VarId> inputs{engine->makeIntVar(lb, lb, ub),
                              engine->makeIntVar(lb, lb, ub),
                              engine->makeIntVar(lb, lb, ub)};
    const VarId violationId = engine->makeIntVar(0, 0, 2);
    BoolAllEqual& invariant = engine->makeConstraint<BoolAllEqual>(
        *engine, violationId, std::vector<VarId>(inputs));
    engine->close();

    Timestamp ts = engine->currentTimestamp();

    for (Int val = lb; val <= ub; ++val) {
      ++ts;
      for (size_t i = 0; i < inputs.size(); ++i) {
        engine->setValue(ts, inputs[i], val);
        const Int expectedViolation = computeViolation(ts, inputs);

        invariant.notifyInputChanged(ts, LocalId(i));
        EXPECT_EQ(expectedViolation, engine->value(ts, violationId));
      }
    }
  }
}

TEST_F(BoolAllEqualTest, NextInput) {
  const size_t numInputs = 1000;
  const Int lb = 0;
  const Int ub = numInputs - 1;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(engine->makeIntVar(i, lb, ub));
  }
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId violationId = engine->makeIntVar(0, 0, 2);
  BoolAllEqual& invariant = engine->makeConstraint<BoolAllEqual>(
      *engine, violationId, std::vector<VarId>(inputs));
  engine->close();

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

TEST_F(BoolAllEqualTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const size_t numInputs = 100;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(engine->makeIntVar(valueDist(gen), lb, ub));
  }
  const VarId violationId = engine->makeIntVar(0, 0, numInputs - 1);
  BoolAllEqual& invariant = engine->makeConstraint<BoolAllEqual>(
      *engine, violationId, std::vector<VarId>(inputs));
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
      EXPECT_EQ(engine->value(ts, violationId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(BoolAllEqualTest, Commit) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const size_t numInputs = 1000;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<size_t> varDist(size_t(0), numInputs);
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(valueDist(gen));
    inputs.emplace_back(engine->makeIntVar(committedValues.back(), lb, ub));
  }
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarId violationId = engine->makeIntVar(0, 0, 2);
  BoolAllEqual& invariant = engine->makeConstraint<BoolAllEqual>(
      *engine, violationId, std::vector<VarId>(inputs));
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), violationId),
            computeViolation(engine->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
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
    const Int notifiedViolation = engine->value(ts, violationId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, engine->value(ts, violationId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, violationId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, engine->value(ts + 1, violationId));
  }
}

class MockAllDifferent : public BoolAllEqual {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolAllEqual::registerVars();
  }
  explicit MockAllDifferent(Engine& engine, VarId violationId,
                            std::vector<VarId> t_variables)
      : BoolAllEqual(engine, violationId, t_variables) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolAllEqual::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolAllEqual::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolAllEqual::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolAllEqual::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolAllEqual::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(BoolAllEqualTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    const size_t numArgs = 10;
    for (size_t value = 0; value < numArgs; ++value) {
      args.emplace_back(engine->makeIntVar(0, -100, 100));
    }
    const VarId viol = engine->makeIntVar(0, 0, static_cast<Int>(numArgs));
    testNotifications<MockAllDifferent>(
        &engine->makeConstraint<MockAllDifferent>(*engine, viol, args),
        {propMode, markingMode, numArgs + 1, args.front(), 1, viol});
  }
}

}  // namespace atlantis::testing
