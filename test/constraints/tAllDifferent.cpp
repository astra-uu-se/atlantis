#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class AllDifferentTest : public InvariantTest {
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
    std::vector<bool> checked(values.size(), false);
    Int expectedViolation = 0;
    for (size_t i = 0; i < values.size(); ++i) {
      if (checked[i]) {
        continue;
      }
      checked[i] = true;
      for (size_t j = i + 1; j < values.size(); ++j) {
        if (checked[j]) {
          continue;
        }
        if (values[i] == values[j]) {
          checked[j] = true;
          ++expectedViolation;
        }
      }
    }
    return expectedViolation;
  }
};

TEST_F(AllDifferentTest, Recompute) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_TRUE(lb <= ub);
    engine->open();
    const VarId a = engine->makeIntVar(lb, lb, ub);
    const VarId b = engine->makeIntVar(lb, lb, ub);
    const VarId c = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntVar(0, 0, 2);
    AllDifferent& invariant = engine->makeConstraint<AllDifferent>(
        violationId, std::vector<VarId>{a, b, c});
    engine->close();

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          engine->setValue(engine->currentTimestamp(), a, aVal);
          engine->setValue(engine->currentTimestamp(), b, bVal);
          engine->setValue(engine->currentTimestamp(), c, cVal);
          const Int expectedViolation =
              computeViolation(std::vector{aVal, bVal, cVal});
          invariant.recompute(engine->currentTimestamp(), *engine);
          EXPECT_EQ(expectedViolation,
                    engine->value(engine->currentTimestamp(), violationId));
        }
      }
    }
  }
}

TEST_F(AllDifferentTest, NotifyInputChanged) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_TRUE(lb <= ub);

    engine->open();
    std::vector<VarId> inputs{engine->makeIntVar(lb, lb, ub),
                              engine->makeIntVar(lb, lb, ub),
                              engine->makeIntVar(lb, lb, ub)};
    const VarId violationId = engine->makeIntVar(0, 0, 2);
    AllDifferent& invariant = engine->makeConstraint<AllDifferent>(
        violationId, std::vector<VarId>(inputs));
    engine->close();

    for (Int val = lb; val <= ub; ++val) {
      for (size_t i = 0; i < inputs.size(); ++i) {
        engine->setValue(engine->currentTimestamp(), inputs[i], val);
        const Int expectedViolation =
            computeViolation(engine->currentTimestamp(), inputs);

        invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                     LocalId(i));
        EXPECT_EQ(expectedViolation,
                  engine->value(engine->currentTimestamp(), violationId));
      }
    }
  }
}

TEST_F(AllDifferentTest, NextInput) {
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
  AllDifferent& invariant = engine->makeConstraint<AllDifferent>(
      violationId, std::vector<VarId>(inputs));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
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

TEST_F(AllDifferentTest, NotifyCurrentInputChanged) {
  const Int lb = -10;
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
  AllDifferent& invariant = engine->makeConstraint<AllDifferent>(
      violationId, std::vector<VarId>(inputs));
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

TEST_F(AllDifferentTest, Commit) {
  const Int lb = -10;
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
  AllDifferent& invariant = engine->makeConstraint<AllDifferent>(
      violationId, std::vector<VarId>(inputs));
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

class MockAllDifferent : public AllDifferent {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    AllDifferent::registerVars(engine);
  }
  MockAllDifferent(VarId violationId, std::vector<VarId> t_variables)
      : AllDifferent(violationId, t_variables) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return AllDifferent::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return AllDifferent::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          AllDifferent::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          AllDifferent::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      AllDifferent::commit(t, engine);
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

TEST_F(AllDifferentTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    const Int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.emplace_back(engine->makeIntVar(0, -100, 100));
    }
    const VarId viol = engine->makeIntVar(0, 0, numArgs);
    testNotifications<MockAllDifferent>(
        &engine->makeInvariant<MockAllDifferent>(viol, args), propMode,
        markingMode, numArgs + 1, args.front(), 1, viol);
  }
}

}  // namespace
