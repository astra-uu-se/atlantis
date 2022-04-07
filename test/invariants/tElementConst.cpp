
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/elementConst.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class ElementConstTest : public InvariantTest {
 protected:
  const size_t numValues = 1000;
  const Int valueLb = std::numeric_limits<Int>::min();
  const Int valueUb = std::numeric_limits<Int>::max();
  Int indexLb = 0;
  Int indexUb = numValues - 1;
  std::vector<Int> values;
  std::uniform_int_distribution<Int> valueDist;
  std::uniform_int_distribution<Int> indexDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    values.resize(numValues, 0);
    valueDist = std::uniform_int_distribution<Int>(valueLb, valueUb);
    indexDist = std::uniform_int_distribution<Int>(indexLb, indexUb);
    for (size_t i = 0; i < values.size(); ++i) {
      values.at(i) = valueDist(gen);
    }
  }

  void TearDown() override {
    InvariantTest::TearDown();
    values.clear();
  }

  Int computeOutput(const Timestamp ts, const VarId index) {
    return computeOutput(engine->value(ts, index));
  }

  Int computeOutput(const Int indexVal) { return values.at(indexVal); }
};

TEST_F(ElementConstTest, Recompute) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntVar(valueLb, valueLb, valueUb);
  ElementConst& invariant = engine->makeInvariant<ElementConst>(
      index, std::vector<Int>(values), outputId);
  engine->close();

  for (Int val = indexLb; val <= indexUb; ++val) {
    engine->setValue(engine->currentTimestamp(), index, val);
    EXPECT_EQ(engine->value(engine->currentTimestamp(), index), val);

    const Int expectedOutput = computeOutput(engine->currentTimestamp(), index);
    invariant.recompute(engine->currentTimestamp(), *engine);
    EXPECT_EQ(engine->value(engine->currentTimestamp(), index), val);

    EXPECT_EQ(expectedOutput,
              engine->value(engine->currentTimestamp(), outputId));
  }
}

TEST_F(ElementConstTest, NotifyInputChanged) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntVar(valueLb, valueLb, valueUb);
  ElementConst& invariant = engine->makeInvariant<ElementConst>(
      index, std::vector<Int>(values), outputId);
  engine->close();

  for (Int val = indexLb; val <= indexUb; ++val) {
    engine->setValue(engine->currentTimestamp(), index, val);
    const Int expectedOutput = computeOutput(engine->currentTimestamp(), index);

    invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                 LocalId(0));
    EXPECT_EQ(expectedOutput,
              engine->value(engine->currentTimestamp(), outputId));
  }
}

TEST_F(ElementConstTest, NextInput) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntVar(valueLb, valueLb, valueUb);
  ElementConst& invariant = engine->makeInvariant<ElementConst>(
      index, std::vector<Int>(values), outputId);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    const VarId varId = invariant.nextInput(ts, *engine);
    EXPECT_EQ(varId, index);
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
  }
}

TEST_F(ElementConstTest, NotifyCurrentInputChanged) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  engine->open();
  const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
  const VarId outputId = engine->makeIntVar(valueLb, valueLb, valueUb);
  ElementConst& invariant = engine->makeInvariant<ElementConst>(
      index, std::vector<Int>(values), outputId);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    const VarId curInput = invariant.nextInput(ts, *engine);
    EXPECT_EQ(curInput, index);

    const Int oldVal = engine->value(ts, curInput);
    do {
      engine->setValue(ts, curInput, indexDist(gen));
    } while (engine->value(ts, curInput) == oldVal);

    invariant.notifyCurrentInputChanged(ts, *engine);

    EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, index));
  }
}

TEST_F(ElementConstTest, Commit) {
  EXPECT_TRUE(valueLb <= valueUb);
  EXPECT_TRUE(indexLb <= indexUb);

  std::vector<Int> indexValues(numValues, 0);
  for (size_t i = 0; i < indexValues.size(); ++i) {
    indexValues.at(i) = i;
  }

  std::shuffle(indexValues.begin(), indexValues.end(), rng);

  engine->open();
  const VarId index = engine->makeIntVar(indexValues.back(), indexLb, indexUb);
  const VarId outputId = engine->makeIntVar(valueLb, valueLb, valueUb);
  ElementConst& invariant = engine->makeInvariant<ElementConst>(
      index, std::vector<Int>(values), outputId);
  engine->close();

  Int committedValue = engine->committedValue(index);

  for (size_t i = 0; i < indexValues.size(); ++i) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(1 + i);
    ASSERT_EQ(engine->committedValue(index), committedValue);

    engine->setValue(ts, index, indexValues[i]);

    // notify changes
    invariant.notifyInputChanged(ts, *engine, LocalId(0));

    // incremental value
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, index);
    committedValue = engine->value(ts, index);
    engine->commitIf(ts, outputId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

class MockElementConst : public ElementConst {
 public:
  bool initialized = false;
  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    ElementConst::init(timestamp, engine);
  }
  MockElementConst(VarId i, std::vector<Int> X, VarId b)
      : ElementConst(i, X, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return ElementConst::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return ElementConst::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          ElementConst::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          ElementConst::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      ElementConst::commit(t, engine);
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
TEST_F(ElementConstTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<Int> args;
    const Int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(value);
    }
    const VarId idx = engine->makeIntVar(0, 0, numArgs - 1);
    const VarId output = engine->makeIntVar(-10, -100, 100);
    testNotifications<MockElementConst>(
        &engine->makeInvariant<MockElementConst>(idx, args, output), propMode,
        markingMode, 2, idx, 5, output);
  }
}

}  // namespace
