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
    ON_CALL(*this, getNextInput)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return Linear::getNextInput(ts, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          Linear::notifyCurrentInputChanged(ts, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          Linear::notifyIntChanged(ts, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      Linear::commit(ts, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, getNextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyIntChanged,
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

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    std::vector<VarId> args{};
    int numArgs = 10;
    Int sum = 0;
    for (Int value = 1; value <= numArgs; ++value) {
      args.push_back(engine->makeIntVar(value, 1, numArgs));
      sum += value;
    }

    VarId output = engine->makeIntVar(-10, -100, numArgs * numArgs);

    auto& invariant =
        engine->makeInvariant<MockLinear>(std::vector<VarId>{args}, output);

    EXPECT_TRUE(invariant.initialized);

    EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

    EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->propagationMode ==
        PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(invariant, getNextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(invariant, getNextInput(testing::_, testing::_))
          .Times(numArgs + 1);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(args[0], 5);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();
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

  RC_ASSERT(engine->getCommittedValue(d) ==
            aCoef * aVal + bCoef * bVal + cCoef * cVal);
}

/**
 *  Testing constructor
 */

TEST_F(LinearTest, Init) {
  EXPECT_EQ(engine->getCommittedValue(d), -39);
  EXPECT_EQ(engine->getValue(engine->getTmpTimestamp(d), d), -39);
}

TEST_F(LinearTest, Recompute) {
  EXPECT_EQ(engine->getValue(0, d), -39);
  EXPECT_EQ(engine->getCommittedValue(d), -39);

  Timestamp newTimestamp = 1;
  engine->setValue(newTimestamp, a, 40);
  linear->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->getCommittedValue(d), -39);
  EXPECT_EQ(engine->getValue(newTimestamp, d), 0);
}

TEST_F(LinearTest, NotifyChange) {
  EXPECT_EQ(engine->getValue(0, d), -39);  // initially the value of d is -39

  Timestamp time1 = 1;

  EXPECT_EQ(engine->getValue(time1, a), 1);
  engine->setValue(time1, a, 40);
  EXPECT_EQ(engine->getCommittedValue(a), 1);
  EXPECT_EQ(engine->getValue(time1, a), 40);
  linear->notifyIntChanged(time1, *engine, 0);
  EXPECT_EQ(engine->getValue(time1, d), 0);  // incremental value of d is 0;

  engine->setValue(time1, b, 0);
  linear->notifyIntChanged(time1, *engine, 1);
  auto tmpValue = engine->getValue(time1, d);  // incremental value of d is -40;

  // Incremental computation gives the same result as recomputation
  linear->recompute(time1, *engine);
  EXPECT_EQ(engine->getValue(time1, d), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->getValue(time2, a), 1);
  engine->setValue(time2, a, 20);
  EXPECT_EQ(engine->getCommittedValue(a), 1);
  EXPECT_EQ(engine->getValue(time2, a), 20);
  linear->notifyIntChanged(time2, *engine, 0);
  EXPECT_EQ(engine->getValue(time2, d), -20);  // incremental value of d is 0;
}

TEST_F(LinearTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->getValue(0, d), -39);  // initially the value of d is -39
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->getCommittedValue(a), 1);
    ASSERT_EQ(engine->getCommittedValue(b), 2);
    ASSERT_EQ(engine->getCommittedValue(c), 3);
    ASSERT_EQ(engine->getCommittedValue(d),
              -39);  // d is committed by register.

    // Set all variables
    engine->setValue(currentTimestamp, a, distribution(gen));
    engine->setValue(currentTimestamp, b, distribution(gen));
    engine->setValue(currentTimestamp, c, distribution(gen));

    // notify changes
    if (engine->getCommittedValue(a) != engine->getValue(currentTimestamp, a)) {
      linear->notifyIntChanged(currentTimestamp, *engine, 0);
    }
    if (engine->getCommittedValue(b) != engine->getValue(currentTimestamp, b)) {
      linear->notifyIntChanged(currentTimestamp, *engine, 1);
    }
    if (engine->getCommittedValue(c) != engine->getValue(currentTimestamp, c)) {
      linear->notifyIntChanged(currentTimestamp, *engine, 2);
    }

    // incremental value
    auto tmp = engine->getValue(currentTimestamp, d);
    linear->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->getValue(currentTimestamp, d));
  }
}

TEST_F(LinearTest, Commit) {
  EXPECT_EQ(engine->getCommittedValue(d), -39);

  Timestamp currentTimestamp = 1;

  engine->setValue(currentTimestamp, a, 40);
  engine->setValue(currentTimestamp, b,
                   2);  // This change is not notified and should
                        // not have an impact on the commit

  linear->notifyIntChanged(currentTimestamp, *engine, 0);
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

  auto& invariant =
      engine->makeInvariant<MockLinear>(std::vector<VarId>{args}, output);

  EXPECT_TRUE(invariant.initialized);

  EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(output), sum);
}

TEST_F(LinearTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(LinearTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

TEST_F(LinearTest, NotificationsMixed) {
  testNotifications(PropagationEngine::PropagationMode::MIXED);
}

}  // namespace
