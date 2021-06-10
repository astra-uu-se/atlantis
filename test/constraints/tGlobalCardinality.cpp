#include <algorithm>
#include <limits>
#include <random>
#include <vector>

#include "constraints/globalCardinality.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "variables/savedInt.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockGlobalCardinality : public GlobalCardinality {
 public:
  bool m_initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    m_initialized = true;
    GlobalCardinality::init(timestamp, engine);
  }

  MockGlobalCardinality(VarId violationId, std::vector<VarId>&& t_variables,
                        std::vector<Int>&& cover, std::vector<Int>&& t_counts,
                        bool closed)
      : GlobalCardinality(violationId, std::vector<VarId>{t_variables},
                          std::vector<Int>{cover}, std::vector<Int>{t_counts},
                          closed) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return GlobalCardinality::recompute(timestamp, engine);
        });
    ON_CALL(*this, getNextDependency)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return GlobalCardinality::getNextDependency(t, engine);
        });

    ON_CALL(*this, notifyCurrentDependencyChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          GlobalCardinality::notifyCurrentDependencyChanged(t, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          GlobalCardinality::notifyIntChanged(t, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      GlobalCardinality::commit(t, engine);
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

class GlobalCardinalityTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  std::shared_ptr<GlobalCardinality> globalCardinality;
  std::mt19937 gen;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    a = engine->makeIntVar(1, -100, 100);
    b = engine->makeIntVar(2, -100, 100);
    c = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 3);

    globalCardinality = engine->makeConstraint<GlobalCardinality>(
        violationId, std::vector<VarId>({a, b, c}), std::vector<Int>({1, 2, 3}),
        std::vector<Int>({1, 1, 1}));
    engine->close();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    std::vector<VarId> args{};
    Int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(0, -100, 100));
    }

    VarId viol = engine->makeIntVar(0, 0, numArgs);

    auto invariant = engine->makeInvariant<MockGlobalCardinality>(
        viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
        std::vector<Int>{1, 2, 3}, false);

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
          .Times(numArgs + 1);
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
    engine->setValue(args.at(0), 1);
    engine->endMove();

    engine->beginQuery();
    engine->query(viol);
    engine->endQuery();
  }
};

/**
 *  Testing constructor
 */

TEST_F(GlobalCardinalityTest, Init) {
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(engine->getTmpTimestamp(violationId), violationId),
            1);
}

TEST_F(GlobalCardinalityTest, Recompute) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);

  Timestamp newTime = 1;

  engine->setValue(newTime, c, 3);
  globalCardinality->recompute(newTime, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(newTime, violationId), 0);

  engine->setValue(newTime, a, 2);
  globalCardinality->recompute(newTime, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(newTime, violationId), 1);
}

TEST_F(GlobalCardinalityTest, NotifyChange) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);

  Timestamp time1 = 1;

  EXPECT_EQ(engine->getValue(time1, a), 1);
  engine->setValue(time1, a, 2);
  EXPECT_EQ(engine->getCommittedValue(a), 1);
  EXPECT_EQ(engine->getValue(time1, a), 2);
  globalCardinality->notifyIntChanged(time1, *engine, 0);
  EXPECT_EQ(engine->getValue(time1, violationId), 2);

  engine->setValue(time1, b, 3);
  globalCardinality->notifyIntChanged(time1, *engine, 1);
  auto tmpValue = engine->getValue(time1, violationId);
  EXPECT_EQ(tmpValue, 1);

  // Incremental computation gives the same result as recomputation
  globalCardinality->recompute(time1, *engine);
  EXPECT_EQ(engine->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->getValue(time2, b), 2);
  engine->setValue(time2, b, 20);
  EXPECT_EQ(engine->getCommittedValue(b), 2);
  EXPECT_EQ(engine->getValue(time2, b), 20);
  globalCardinality->notifyIntChanged(time2, *engine, 1);
  EXPECT_EQ(engine->getValue(time2, violationId), 1);
}

TEST_F(GlobalCardinalityTest, IncrementalVsRecompute) {
  for (bool closed : std::vector<bool>{false, true}) {
    EXPECT_EQ(engine->getValue(0, violationId),
              1);  // initially the value of violationId is 0
    // todo: not clear if we actually want to deal with overflows...
    std::uniform_int_distribution<> distribution(closed ? 1 : -100,
                                                 closed ? 3 : 100);

    Timestamp currentTime = 1;
    for (size_t i = 0; i < 100; ++i) {
      ++currentTime;
      // Check that we do not accidentally commit
      ASSERT_EQ(engine->getCommittedValue(a), 1);
      ASSERT_EQ(engine->getCommittedValue(b), 2);
      ASSERT_EQ(engine->getCommittedValue(violationId),
                1);  // violationId is commited by register.

      // Set all variables
      engine->setValue(currentTime, a, distribution(gen));
      engine->setValue(currentTime, b, distribution(gen));

      // notify changes
      if (engine->getCommittedValue(a) != engine->getValue(currentTime, a)) {
        globalCardinality->notifyIntChanged(currentTime, *engine, 0);
      }
      if (engine->getCommittedValue(b) != engine->getValue(currentTime, b)) {
        globalCardinality->notifyIntChanged(currentTime, *engine, 1);
      }

      // incremental value
      auto tmp = engine->getValue(currentTime, violationId);
      globalCardinality->recompute(currentTime, *engine);

      ASSERT_EQ(tmp, engine->getValue(currentTime, violationId));
    }
  }
}

TEST_F(GlobalCardinalityTest, Commit) {
  /*
  It is difficult to test the method commit as it only
  commits the internal data structures of the constraint.
  The internal data structures are (in almost all cases)
  private.
  */
  ASSERT_TRUE(true);
}

TEST_F(GlobalCardinalityTest, CreateGlobalCardinality) {
  engine->open();

  std::vector<VarId> args{};
  Int numArgs = 10;
  for (Int value = 0; value < numArgs; ++value) {
    args.push_back(engine->makeIntVar(1, -100, 100));
  }

  VarId viol = engine->makeIntVar(0, 0, numArgs);

  auto invariant = engine->makeInvariant<MockGlobalCardinality>(
      viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
      std::vector<Int>{1, 2, 3}, false);

  EXPECT_TRUE(invariant->m_initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(viol), numArgs - 1);
}

TEST_F(GlobalCardinalityTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(GlobalCardinalityTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
