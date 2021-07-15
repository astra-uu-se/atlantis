#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockAllDifferent : public AllDifferent {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    AllDifferent::init(timestamp, engine);
  }

  MockAllDifferent(VarId violationId, std::vector<VarId>&& t_variables)
      : AllDifferent(violationId, std::vector<VarId>{t_variables}) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return AllDifferent::recompute(timestamp, engine);
        });
    ON_CALL(*this, getNextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return AllDifferent::getNextInput(t, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          AllDifferent::notifyCurrentInputChanged(t, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          AllDifferent::notifyIntChanged(t, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      AllDifferent::commit(t, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, getNextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyIntChanged, (Timestamp t, Engine& engine, LocalId id),
              (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class AllDifferentTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  std::shared_ptr<AllDifferent> allDifferent;
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

    allDifferent = engine->makeConstraint<AllDifferent>(
        violationId, std::vector<VarId>({a, b, c}));
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

    auto invariant =
        engine->makeInvariant<MockAllDifferent>(viol, std::vector<VarId>{args});

    EXPECT_TRUE(invariant->initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->propagationMode ==
        PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_))
          .Times(numArgs + 1);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
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

TEST_F(AllDifferentTest, Init) {
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(engine->getTmpTimestamp(violationId), violationId),
            1);
}

TEST_F(AllDifferentTest, Recompute) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);

  Timestamp newTimestamp = 1;

  engine->setValue(newTimestamp, c, 3);
  allDifferent->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(newTimestamp, violationId), 0);

  engine->setValue(newTimestamp, a, 2);
  allDifferent->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->getCommittedValue(violationId), 1);
  EXPECT_EQ(engine->getValue(newTimestamp, violationId), 1);
}

TEST_F(AllDifferentTest, NotifyChange) {
  EXPECT_EQ(engine->getValue(0, violationId), 1);

  Timestamp time1 = 1;

  EXPECT_EQ(engine->getValue(time1, a), 1);
  engine->setValue(time1, a, 2);
  EXPECT_EQ(engine->getCommittedValue(a), 1);
  EXPECT_EQ(engine->getValue(time1, a), 2);
  allDifferent->notifyIntChanged(time1, *engine, 0);
  EXPECT_EQ(engine->getValue(time1, violationId), 2);

  engine->setValue(time1, b, 3);
  allDifferent->notifyIntChanged(time1, *engine, 1);
  auto tmpValue = engine->getValue(time1, violationId);

  // Incremental computation gives the same result as recomputation
  allDifferent->recompute(time1, *engine);
  EXPECT_EQ(engine->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->getValue(time2, b), 2);
  engine->setValue(time2, b, 20);
  EXPECT_EQ(engine->getCommittedValue(b), 2);
  EXPECT_EQ(engine->getValue(time2, b), 20);
  allDifferent->notifyIntChanged(time2, *engine, 1);
  EXPECT_EQ(engine->getValue(time2, violationId), 0);
}

TEST_F(AllDifferentTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->getValue(0, violationId),
            1);  // initially the value of violationId is 0
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100, 100);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 2; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->getCommittedValue(a), 1);
    ASSERT_EQ(engine->getCommittedValue(b), 2);
    ASSERT_EQ(engine->getCommittedValue(violationId),
              1);  // violationId is commited by register.

    // Set all variables
    engine->setValue(currentTimestamp, a, distribution(gen));
    engine->setValue(currentTimestamp, b, distribution(gen));

    // notify changes
    if (engine->getCommittedValue(a) != engine->getValue(currentTimestamp, a)) {
      allDifferent->notifyIntChanged(currentTimestamp, *engine, 0);
    }
    if (engine->getCommittedValue(b) != engine->getValue(currentTimestamp, b)) {
      allDifferent->notifyIntChanged(currentTimestamp, *engine, 1);
    }

    // incremental value
    auto tmp = engine->getValue(currentTimestamp, violationId);
    allDifferent->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->getValue(currentTimestamp, violationId));
  }
}

TEST_F(AllDifferentTest, Commit) {
  /*
  It is difficult to test the method commit as it only
  commits the internal data structures of the constraint.
  The internal data structures are (in almost all cases)
  private.
  */
  ASSERT_TRUE(true);
}

TEST_F(AllDifferentTest, CreateAllDifferent) {
  engine->open();

  std::vector<VarId> args{};
  Int numArgs = 10;
  for (Int value = 0; value < numArgs; ++value) {
    args.push_back(engine->makeIntVar(0, -100, 100));
  }

  VarId viol = engine->makeIntVar(0, 0, numArgs);

  auto invariant =
      engine->makeInvariant<MockAllDifferent>(viol, std::vector<VarId>{args});

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(viol), numArgs - 1);
}

TEST_F(AllDifferentTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(AllDifferentTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

TEST_F(AllDifferentTest, NotificationsMixed) {
  testNotifications(PropagationEngine::PropagationMode::MIXED);
}

}  // namespace
