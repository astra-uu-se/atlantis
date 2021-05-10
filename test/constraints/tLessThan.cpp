#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "constraints/lessThan.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "variables/savedInt.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockLessThan : public LessThan {
 public:
  bool m_initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    m_initialized = true;
    LessThan::init(timestamp, engine);
  }

  MockLessThan(VarId violationId, VarId a, VarId b)
      : LessThan(violationId, a, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return LessThan::recompute(timestamp, engine);
        });
    ON_CALL(*this, getNextDependency)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return LessThan::getNextDependency(t, engine);
        });

    ON_CALL(*this, notifyCurrentDependencyChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          LessThan::notifyCurrentDependencyChanged(t, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          LessThan::notifyIntChanged(t, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      LessThan::commit(t, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyIntChanged, (Timestamp t, Engine& engine, LocalId id),
              (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class LessThanTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId x = NULL_ID;
  VarId y = NULL_ID;
  std::shared_ptr<LessThan> lessThan;
  std::mt19937 gen;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    x = engine->makeIntVar(2, -100, 100);
    y = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 200);

    lessThan = engine->makeConstraint<LessThan>(violationId, x, y);
    engine->close();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(5, -100, 100);
    VarId b = engine->makeIntVar(0, -100, 100);

    VarId viol = engine->makeIntVar(0, 0, 200);

    auto invariant = engine->makeInvariant<MockLessThan>(viol, a, b);

    EXPECT_TRUE(invariant->m_initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->mode == PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_))
          .Times(0);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else if (engine->mode ==
               PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
      EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_))
          .Times(3);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
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

TEST_F(LessThanTest, Init) {
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(engine->getTmpTimestamp(violationId), violationId),
            1);
}

TEST_F(LessThanTest, Recompute) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);

  Timestamp time1 = 1;
  Timestamp time2 = time1 + 1;

  engine->setValue(time1, x, 40);
  lessThan->recompute(time1, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(time1, violationId), 39);

  engine->setValue(time2, y, 20);
  lessThan->recompute(time2, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(time2, violationId), 0);
}

TEST_F(LessThanTest, NonViolatingUpdate) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);

  Timestamp time;

  for (size_t i = 0; i < 10000; ++i) {
    time = Timestamp(1 + i);
    engine->setValue(time, x, 1 - i);
    lessThan->recompute(time, *engine);
    EXPECT_EQ(engine->getCommittedValue(violationId), 1);
    EXPECT_EQ(engine->getValue(time, violationId), 0);
  }

  for (size_t i = 0; i < 10000; ++i) {
    time = Timestamp(1 + i);
    engine->setValue(time, y, 5 + i);
    lessThan->recompute(time, *engine);
    EXPECT_EQ(engine->getCommittedValue(violationId), 1);
    EXPECT_EQ(engine->getValue(time, violationId), 0);
  }
}

TEST_F(LessThanTest, NotifyChange) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(engine->getValue(time1, x), 2);  // Variable has not changed.
  engine->setValue(time1, x, 40);
  EXPECT_EQ(engine->getCommittedValue(x), 2);
  EXPECT_EQ(engine->getValue(time1, x), 40);
  lessThan->notifyIntChanged(time1, *engine,
                             unused);  // Runs recompute internally
  EXPECT_EQ(engine->getValue(time1, violationId), 39);

  engine->setValue(time1, y, 0);
  lessThan->notifyIntChanged(time1, *engine,
                             unused);  // Runs recompute internally
  auto tmpValue = engine->getValue(time1, violationId);  // 41

  // Incremental computation gives the same result as recomputation
  lessThan->recompute(time1, *engine);
  EXPECT_EQ(engine->getValue(time1, violationId), tmpValue);
  EXPECT_EQ(tmpValue, 41);

  Timestamp time2 = time1 + 1;
  EXPECT_EQ(engine->getValue(time2, y), 2);
  engine->setValue(time2, y, 20);
  EXPECT_EQ(engine->getCommittedValue(y), 2);
  EXPECT_EQ(engine->getValue(time2, y), 20);
  lessThan->notifyIntChanged(time2, *engine, unused);
  EXPECT_EQ(engine->getValue(time2, violationId), 0);
}

TEST_F(LessThanTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->getValue(0, violationId),
            1);  // initially the value of violationId is 1
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTime;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->getCommittedValue(x), 2);
    ASSERT_EQ(engine->getCommittedValue(y), 2);
    ASSERT_EQ(engine->getCommittedValue(violationId),
              1);  // violationId is committed by register.

    // Set all variables
    engine->setValue(currentTime, x, distribution(gen));
    engine->setValue(currentTime, y, distribution(gen));

    // notify changes
    if (engine->getCommittedValue(x) != engine->getValue(currentTime, x)) {
      lessThan->notifyIntChanged(currentTime, *engine, unused);
    }
    if (engine->getCommittedValue(y) != engine->getValue(currentTime, y)) {
      lessThan->notifyIntChanged(currentTime, *engine, unused);
    }

    // incremental value
    auto tmp = engine->getValue(currentTime, violationId);
    lessThan->recompute(currentTime, *engine);

    ASSERT_EQ(tmp, engine->getValue(currentTime, violationId));
  }
}

TEST_F(LessThanTest, Commit) {
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);

  LocalId unused = -1;

  Timestamp currentTime = 1;

  engine->setValue(currentTime, x, 40);
  engine->setValue(currentTime, y,
                   2);  // This change is not notified and should
                        // not have an impact on the commit

  lessThan->notifyIntChanged(currentTime, *engine, unused);
}

TEST_F(LessThanTest, CreateLessThan) {
  engine->open();

  VarId a = engine->makeIntVar(5, -100, 100);
  VarId b = engine->makeIntVar(0, -100, 100);

  VarId viol = engine->makeIntVar(0, 0, 201);

  auto invariant = engine->makeInvariant<MockLessThan>(viol, a, b);

  EXPECT_TRUE(invariant->m_initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(viol), 6);
}

TEST_F(LessThanTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(LessThanTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
