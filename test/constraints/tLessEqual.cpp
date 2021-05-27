#include <limits>
#include <random>
#include <vector>

#include "constraints/lessEqual.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockLessEqual : public LessEqual {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    LessEqual::init(timestamp, engine);
  }

  MockLessEqual(VarId violationId, VarId a, VarId b)
      : LessEqual(violationId, a, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return LessEqual::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextParameter)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return LessEqual::nextParameter(ts, engine);
        });

    ON_CALL(*this, notifyCurrentParameterChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          LessEqual::notifyCurrentParameterChanged(ts, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          LessEqual::notifyIntChanged(ts, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      LessEqual::commit(ts, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, nextParameter, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentParameterChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyIntChanged,
              (Timestamp ts, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class LessEqualTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId x = NULL_ID;
  VarId y = NULL_ID;
  std::shared_ptr<LessEqual> lessEqual;
  std::mt19937 gen;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    x = engine->makeIntVar(2, -100, 100);
    y = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 200);

    lessEqual = engine->makeConstraint<LessEqual>(violationId, x, y);
    engine->close();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(5, -100, 100);
    VarId b = engine->makeIntVar(0, -100, 100);

    VarId viol = engine->makeIntVar(0, 0, 200);

    auto invariant = engine->makeInvariant<MockLessEqual>(viol, a, b);

    EXPECT_TRUE(invariant->initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->mode == PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextParameter(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant,
                  notifyCurrentParameterChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else if (engine->mode ==
               PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
      EXPECT_CALL(*invariant, nextParameter(testing::_, testing::_)).Times(3);
      EXPECT_CALL(*invariant,
                  notifyCurrentParameterChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    } else if (engine->mode == PropagationEngine::PropagationMode::MIXED) {
      EXPECT_EQ(0, 1);  // TODO: define the test case for mixed mode.
    }

    engine->beginMove();
    engine->setValue(a, -5);
    engine->endMove();

    engine->beginQuery();
    engine->query(viol);
    engine->endQuery();
  }
};

/**
 *  Testing constructor
 */

TEST_F(LessEqualTest, Init) {
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(violationId), violationId), 0);
}

TEST_F(LessEqualTest, Recompute) {
  EXPECT_EQ(engine->value(0, violationId), 0);
  EXPECT_EQ(engine->committedValue(violationId), 0);

  Timestamp time1 = 1;
  Timestamp time2 = time1 + 1;

  engine->setValue(time1, x, 40);
  lessEqual->recompute(time1, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(time1, violationId), 38);

  engine->setValue(time2, y, 20);
  lessEqual->recompute(time2, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(time2, violationId), 0);
}

TEST_F(LessEqualTest, NonViolatingUpdate) {
  EXPECT_EQ(engine->value(0, violationId), 0);
  EXPECT_EQ(engine->committedValue(violationId), 0);

  Timestamp timestamp;

  for (size_t i = 0; i < 10000; ++i) {
    timestamp = Timestamp(1 + i);
    engine->setValue(timestamp, x, 1 - i);
    lessEqual->recompute(timestamp, *engine);
    EXPECT_EQ(engine->committedValue(violationId), 0);
    EXPECT_EQ(engine->value(timestamp, violationId), 0);
  }

  for (size_t i = 0; i < 10000; ++i) {
    timestamp = Timestamp(1 + i);
    engine->setValue(timestamp, y, 5 + i);
    lessEqual->recompute(timestamp, *engine);
    EXPECT_EQ(engine->committedValue(violationId), 0);
    EXPECT_EQ(engine->value(timestamp, violationId), 0);
  }
}

TEST_F(LessEqualTest, NotifyChange) {
  EXPECT_EQ(engine->value(0, violationId),
            0);  // initially the value of violationId is 0

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, x), 2);
  engine->setValue(time1, x, 40);
  EXPECT_EQ(engine->committedValue(x), 2);
  EXPECT_EQ(engine->value(time1, x), 40);
  lessEqual->notifyIntChanged(time1, *engine, unused);
  EXPECT_EQ(engine->value(time1, violationId),
            38);  // incremental value of violationId is 0;

  engine->setValue(time1, y, 0);
  lessEqual->notifyIntChanged(time1, *engine, unused);
  auto tmpValue = engine->value(
      time1, violationId);  // incremental value of violationId is 40;

  // Incremental computation gives the same result as recomputation
  lessEqual->recompute(time1, *engine);
  EXPECT_EQ(engine->value(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->value(time2, y), 2);
  engine->setValue(time2, y, 20);
  EXPECT_EQ(engine->committedValue(y), 2);
  EXPECT_EQ(engine->value(time2, y), 20);
  lessEqual->notifyIntChanged(time2, *engine, unused);
  EXPECT_EQ(engine->value(time2, violationId),
            0);  // incremental value of violationId is 0;
}

TEST_F(LessEqualTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->value(0, violationId),
            0);  // initially the value of violationId is 0
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->committedValue(x), 2);
    ASSERT_EQ(engine->committedValue(y), 2);
    ASSERT_EQ(engine->committedValue(violationId),
              0);  // violationId is committed by register.

    // Set all variables
    engine->setValue(currentTimestamp, x, distribution(gen));
    engine->setValue(currentTimestamp, y, distribution(gen));

    // notify changes
    if (engine->committedValue(x) != engine->value(currentTimestamp, x)) {
      lessEqual->notifyIntChanged(currentTimestamp, *engine, unused);
    }
    if (engine->committedValue(y) != engine->value(currentTimestamp, y)) {
      lessEqual->notifyIntChanged(currentTimestamp, *engine, unused);
    }

    // incremental value
    auto tmp = engine->value(currentTimestamp, violationId);
    lessEqual->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->value(currentTimestamp, violationId));
  }
}

TEST_F(LessEqualTest, Commit) {
  EXPECT_EQ(engine->committedValue(violationId), 0);

  LocalId unused = -1;

  Timestamp currentTimestamp = 1;

  engine->setValue(currentTimestamp, x, 40);
  engine->setValue(currentTimestamp, y,
                   2);  // This change is not notified and should
                        // not have an impact on the commit

  lessEqual->notifyIntChanged(currentTimestamp, *engine, unused);
}

TEST_F(LessEqualTest, CreateLessEqual) {
  engine->open();

  VarId a = engine->makeIntVar(5, -100, 100);
  VarId b = engine->makeIntVar(0, -100, 100);

  VarId viol = engine->makeIntVar(0, 0, 200);

  auto invariant = engine->makeInvariant<MockLessEqual>(viol, a, b);

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->newValue(viol), 5);
}

TEST_F(LessEqualTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(LessEqualTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
