#include <iostream>
#include <random>
#include <vector>

#include "constraints/equal.hpp"
#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class MockEqual : public Equal {
 public:
  bool m_initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    m_initialized = true;
    Equal::init(timestamp, engine);
  }

  MockEqual(VarId violationId, VarId a, VarId b)
      : Equal(violationId, a, b) {
          ON_CALL(*this, recompute)
              .WillByDefault([this](Timestamp timestamp, Engine& engine) {
                return Equal::recompute(timestamp, engine);
              });
          ON_CALL(*this, getNextDependency)
              .WillByDefault([this](Timestamp t, Engine& engine) {
                return Equal::getNextDependency(t, engine);
              });

          ON_CALL(*this, notifyCurrentDependencyChanged)
              .WillByDefault([this](Timestamp t, Engine& engine) {
                Equal::notifyCurrentDependencyChanged(t, engine);
              });

          ON_CALL(*this, notifyIntChanged)
              .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
                Equal::notifyIntChanged(t, engine, id);
              });

          ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
            Equal::commit(t, engine);
          });
  }

MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine), (override));

MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& engine),
            (override));

MOCK_METHOD(void, notifyIntChanged, (Timestamp t, Engine& engine, LocalId id),
            (override));
MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

private:
};

class EqualTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId x = NULL_ID;
  VarId y = NULL_ID;
  std::shared_ptr<Equal> equal;
  std::mt19937 gen;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    x = engine->makeIntVar(2, -100, 100);
    y = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 200);

    equal = engine->makeConstraint<Equal>(violationId, x, y);
    engine->close();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(5, -100, 100);
    VarId b = engine->makeIntVar(0, -100, 100);

    VarId viol = engine->makeIntVar(0, 0, 200);

    auto invariant = engine->makeInvariant<MockEqual>(
        viol, a, b);

    EXPECT_TRUE(invariant->m_initialized);
    
    EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->mode = propMode;

    engine->close();

    if (engine->mode == PropagationEngine::PropagationMode::TOP_DOWN) {
      EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
          .Times(0);
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else if (engine->mode == PropagationEngine::PropagationMode::BOTTOM_UP) {
      EXPECT_CALL(*invariant,
                  getNextDependency(testing::_, testing::_)).Times(3);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(0);
    } else if (engine->mode == PropagationEngine::PropagationMode::MIXED) {
      EXPECT_EQ(0, 1);  // TODO: define the test case for mixed mode.
    }

    engine->beginMove();
    engine->setValue(a, 0);
    engine->endMove();

    engine->beginQuery();
    engine->query(viol);
    engine->endQuery();
  }
};

/**
 *  Testing constructor
 */

TEST_F(EqualTest, Init) {
  EXPECT_EQ(engine->getCommittedValue(violationId), 0);
  EXPECT_EQ(engine->getValue(engine->getTmpTimestamp(violationId), violationId), 0);
}

TEST_F(EqualTest, Recompute) {
  EXPECT_EQ(engine->getValue(0, violationId), 0);
  EXPECT_EQ(engine->getCommittedValue(violationId), 0);

  Timestamp newTime = 1;
  engine->setValue(newTime, x, 40);
  equal->recompute(newTime, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 0);
  EXPECT_EQ(engine->getValue(newTime, violationId), 38);
}

TEST_F(EqualTest, NotifyChange) {
  EXPECT_EQ(engine->getValue(0, violationId),
            0);  // initially the value of violationId is 0

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(engine->getValue(time1, x), 2);
  engine->setValue(time1, x, 40);
  EXPECT_EQ(engine->getCommittedValue(x), 2);
  EXPECT_EQ(engine->getValue(time1, x), 40);
  equal->notifyIntChanged(time1, *engine, unused);
  EXPECT_EQ(engine->getValue(time1, violationId),
            38);  // incremental value of violationId is 0;

  engine->setValue(time1, y, 0);
  equal->notifyIntChanged(time1, *engine, unused);
  auto tmpValue = engine->getValue(
      time1, violationId);  // incremental value of violationId is 40;

  // Incremental computation gives the same result as recomputation
  equal->recompute(time1, *engine);
  EXPECT_EQ(engine->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->getValue(time2, y), 2);
  engine->setValue(time2, y, 20);
  EXPECT_EQ(engine->getCommittedValue(y), 2);
  EXPECT_EQ(engine->getValue(time2, y), 20);
  equal->notifyIntChanged(time2, *engine, unused);
  EXPECT_EQ(engine->getValue(time2, violationId),
            18);  // incremental value of violationId is 0;
}

TEST_F(EqualTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->getValue(0, violationId),
            0);  // initially the value of violationId is 0
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
              0);  // violationId is committed by register.

    // Set all variables
    engine->setValue(currentTime, x, distribution(gen));
    engine->setValue(currentTime, y, distribution(gen));

    // notify changes
    if (engine->getCommittedValue(x) != engine->getValue(currentTime, x)) {
      equal->notifyIntChanged(currentTime, *engine, unused);
    }
    if (engine->getCommittedValue(y) != engine->getValue(currentTime, y)) {
      equal->notifyIntChanged(currentTime, *engine, unused);
    }

    // incremental value
    auto tmp = engine->getValue(currentTime, violationId);
    equal->recompute(currentTime, *engine);

    ASSERT_EQ(tmp, engine->getValue(currentTime, violationId));
  }
}

TEST_F(EqualTest, Commit) {
  EXPECT_EQ(engine->getCommittedValue(violationId), 0);

  LocalId unused = -1;

  Timestamp currentTime = 1;

  engine->setValue(currentTime, x, 40);
  engine->setValue(currentTime, y, 2);  // This change is not notified and should
                                   // not have an impact on the commit

  equal->notifyIntChanged(currentTime, *engine, unused);

  // Committing an invariant does not commit its output!
  // // Commit at wrong timestamp should have no impact
  // equal->commit(currentTime + 1, *engine);
  // EXPECT_EQ(engine->getCommittedValue(violationId), 0);
  // equal->commit(currentTime, *engine);
  // EXPECT_EQ(engine->getCommittedValue(violationId), 38);
}

TEST_F(EqualTest, CreateEqual) {
  engine->open();

  VarId a = engine->makeIntVar(5, -100, 100);
  VarId b = engine->makeIntVar(0, -100, 100);

  VarId viol = engine->makeIntVar(0, 0, 200);

  auto invariant = engine->makeInvariant<MockEqual>(
      viol, a, b);

  EXPECT_TRUE(invariant->m_initialized);
  
  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(viol), 5);
}

TEST_F(EqualTest, NotificationsTopDown) {
  testNotifications(PropagationEngine::PropagationMode::TOP_DOWN);
}

TEST_F(EqualTest, NotificationsBottomUp) {
  testNotifications(PropagationEngine::PropagationMode::BOTTOM_UP);
}

}  // namespace
