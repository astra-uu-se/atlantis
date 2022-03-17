#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/linear.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockLinear : public Linear {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    Linear::init(timestamp, engine);
  }

  MockLinear(std::vector<VarId>&& X, VarId b)
      : Linear(std::vector<VarId>{X}, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Linear::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return Linear::nextInput(ts, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          Linear::notifyCurrentInputChanged(ts, engine);
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          Linear::notifyInputChanged(ts, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      Linear::commit(ts, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp ts, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class LinearTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  VarId d = NULL_ID;
  Int aCoef = 1;
  Int bCoef = 10;
  Int cCoef = -20;
  Linear* linear;
  std::mt19937 gen;

  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    a = engine->makeIntVar(1, -100, 100);
    b = engine->makeIntVar(2, -100, 100);
    c = engine->makeIntVar(3, -100, 100);
    d = engine->makeIntVar(4, -100, 100);

    // d = 1*1+2*10+3*(-20) = 1+20-60 =-39
    linear =
        &(engine->makeInvariant<Linear>(std::vector<Int>({aCoef, bCoef, cCoef}),
                                        std::vector<VarId>({a, b, c}), d));
    engine->close();
  }

  void testNotifications(PropagationMode propMode,
                         OutputToInputMarkingMode markingMode) {
    engine->open();

    std::vector<VarId> args{};
    int numArgs = 10;
    Int sum = 0;
    for (Int value = 1; value <= numArgs; ++value) {
      args.push_back(engine->makeIntVar(value, 1, numArgs));
      sum += value;
    }

    VarId output = engine->makeIntVar(-10, -100, numArgs * numArgs);

    auto invariant = &
        engine->makeInvariant<MockLinear>(std::vector<VarId>{args}, output);

    EXPECT_TRUE(invariant->initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);
    engine->setOutputToInputMarkingMode(markingMode);

    engine->close();

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_))
          .Times(numArgs + 1);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(args[0], 5);
    engine->endMove();

    engine->beginProbe();
    engine->query(output);
    engine->endProbe();
  }
};

RC_GTEST_FIXTURE_PROP(LinearTest, shouldAlwaysBeSum,
                      (Int aVal, Int bVal, Int cVal)) {
  engine->beginMove();
  engine->setValue(a, aVal);
  engine->setValue(b, bVal);
  engine->setValue(c, cVal);
  engine->endMove();

  engine->beginCommit();
  engine->query(d);
  engine->endCommit();

  RC_ASSERT(engine->committedValue(d) ==
            aCoef * aVal + bCoef * bVal + cCoef * cVal);
}

/**
 *  Testing constructor
 */

TEST_F(LinearTest, Init) {
  EXPECT_EQ(engine->committedValue(d), -39);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(d), d), -39);
}

TEST_F(LinearTest, Recompute) {
  EXPECT_EQ(engine->value(0, d), -39);
  EXPECT_EQ(engine->committedValue(d), -39);

  Timestamp newTimestamp = 1;
  engine->setValue(newTimestamp, a, 40);
  linear->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->committedValue(d), -39);
  EXPECT_EQ(engine->value(newTimestamp, d), 0);
}

TEST_F(LinearTest, NotifyChange) {
  EXPECT_EQ(engine->value(0, d), -39);  // initially the value of d is -39

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, a), 1);
  engine->setValue(time1, a, 40);
  EXPECT_EQ(engine->committedValue(a), 1);
  EXPECT_EQ(engine->value(time1, a), 40);
  linear->notifyInputChanged(time1, *engine, 0);
  EXPECT_EQ(engine->value(time1, d), 0);  // incremental value of d is 0;

  engine->setValue(time1, b, 0);
  linear->notifyInputChanged(time1, *engine, 1);
  auto tmpValue = engine->value(time1, d);  // incremental value of d is -40;

  // Incremental computation gives the same result as recomputation
  linear->recompute(time1, *engine);
  EXPECT_EQ(engine->value(time1, d), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->value(time2, a), 1);
  engine->setValue(time2, a, 20);
  EXPECT_EQ(engine->committedValue(a), 1);
  EXPECT_EQ(engine->value(time2, a), 20);
  linear->notifyInputChanged(time2, *engine, 0);
  EXPECT_EQ(engine->value(time2, d), -20);  // incremental value of d is 0;
}

TEST_F(LinearTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->value(0, d), -39);  // initially the value of d is -39
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->committedValue(a), 1);
    ASSERT_EQ(engine->committedValue(b), 2);
    ASSERT_EQ(engine->committedValue(c), 3);
    ASSERT_EQ(engine->committedValue(d),
              -39);  // d is committed by register.

    // Set all variables
    engine->setValue(currentTimestamp, a, distribution(gen));
    engine->setValue(currentTimestamp, b, distribution(gen));
    engine->setValue(currentTimestamp, c, distribution(gen));

    // notify changes
    if (engine->committedValue(a) != engine->value(currentTimestamp, a)) {
      linear->notifyInputChanged(currentTimestamp, *engine, 0);
    }
    if (engine->committedValue(b) != engine->value(currentTimestamp, b)) {
      linear->notifyInputChanged(currentTimestamp, *engine, 1);
    }
    if (engine->committedValue(c) != engine->value(currentTimestamp, c)) {
      linear->notifyInputChanged(currentTimestamp, *engine, 2);
    }

    // incremental value
    auto tmp = engine->value(currentTimestamp, d);
    linear->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->value(currentTimestamp, d));
  }
}

TEST_F(LinearTest, Commit) {
  EXPECT_EQ(engine->committedValue(d), -39);

  Timestamp currentTimestamp = 1;

  engine->setValue(currentTimestamp, a, 40);
  engine->setValue(currentTimestamp, b,
                   2);  // This change is not notified and should
                        // not have an impact on the commit

  linear->notifyInputChanged(currentTimestamp, *engine, 0);
}

TEST_F(LinearTest, CreateLinear) {
  engine->open();

  std::vector<VarId> args{};
  int numArgs = 10;
  Int sum = 0;
  for (Int value = 1; value <= numArgs; ++value) {
    args.push_back(engine->makeIntVar(value, 1, numArgs));
    sum += value;
  }

  VarId output = engine->makeIntVar(-10, -100, numArgs * numArgs);

  auto invariant = &
      engine->makeInvariant<MockLinear>(std::vector<VarId>{args}, output);

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(output), sum);
}

TEST_F(LinearTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(LinearTest, NotificationsOutputToInputNone) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(LinearTest, NotificationsOutputToInputOutputToInputStatic) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(LinearTest, NotificationsOutputToInputInputToOutputExploration) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
