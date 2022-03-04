#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
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

 private:
};

class AllDifferentTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  AllDifferent* allDifferent;
  std::mt19937 gen;

  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    a = engine->makeIntVar(1, -100, 100);
    b = engine->makeIntVar(2, -100, 100);
    c = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 3);

    allDifferent = &(engine->makeConstraint<AllDifferent>(
        violationId, std::vector<VarId>({a, b, c})));
    engine->close();
  }

  void testNotifications(PropagationMode propMode,
                         OutputToInputMarkingMode markingMode) {
    engine->open();

    std::vector<VarId> args{};
    int numArgs = 10;
    args.reserve(numArgs);
    for (int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(0, -100, 100));
    }

    VarId viol = engine->makeIntVar(0, 0, numArgs);

    auto& invariant =
        engine->makeInvariant<MockAllDifferent>(viol, std::vector<VarId>{args});

    EXPECT_TRUE(invariant.initialized);

    EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

    EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);
    engine->setOutputToInputMarkingMode(markingMode);

    engine->close();

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(invariant, nextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(invariant, nextInput(testing::_, testing::_))
          .Times(numArgs + 1);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(args.at(0), 1);
    engine->endMove();

    engine->beginProbe();
    engine->query(viol);
    engine->endProbe();
  }
};

/**
 *  Testing constructor
 */

TEST_F(AllDifferentTest, Init) {
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(violationId), violationId), 1);
}

TEST_F(AllDifferentTest, Recompute) {
  EXPECT_EQ(engine->value(0, violationId), 1);
  EXPECT_EQ(engine->committedValue(violationId), 1);

  Timestamp newTimestamp = 1;

  engine->setValue(newTimestamp, c, 3);
  allDifferent->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(newTimestamp, violationId), 0);

  engine->setValue(newTimestamp, a, 2);
  allDifferent->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(newTimestamp, violationId), 1);
}

TEST_F(AllDifferentTest, NotifyChange) {
  EXPECT_EQ(engine->value(0, violationId), 1);

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, a), 1);
  engine->setValue(time1, a, 2);
  EXPECT_EQ(engine->committedValue(a), 1);
  EXPECT_EQ(engine->value(time1, a), 2);
  allDifferent->notifyInputChanged(time1, *engine, 0);
  EXPECT_EQ(engine->value(time1, violationId), 2);

  engine->setValue(time1, b, 3);
  allDifferent->notifyInputChanged(time1, *engine, 1);
  auto tmpValue = engine->value(time1, violationId);

  // Incremental computation gives the same result as recomputation
  allDifferent->recompute(time1, *engine);
  EXPECT_EQ(engine->value(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->value(time2, b), 2);
  engine->setValue(time2, b, 20);
  EXPECT_EQ(engine->committedValue(b), 2);
  EXPECT_EQ(engine->value(time2, b), 20);
  allDifferent->notifyInputChanged(time2, *engine, 1);
  EXPECT_EQ(engine->value(time2, violationId), 0);
}

TEST_F(AllDifferentTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->value(0, violationId),
            1);  // initially the value of violationId is 0
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100, 100);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 2; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->committedValue(a), 1);
    ASSERT_EQ(engine->committedValue(b), 2);
    ASSERT_EQ(engine->committedValue(violationId),
              1);  // violationId is commited by register.

    // Set all variables
    engine->setValue(currentTimestamp, a, distribution(gen));
    engine->setValue(currentTimestamp, b, distribution(gen));

    // notify changes
    if (engine->committedValue(a) != engine->value(currentTimestamp, a)) {
      allDifferent->notifyInputChanged(currentTimestamp, *engine, 0);
    }
    if (engine->committedValue(b) != engine->value(currentTimestamp, b)) {
      allDifferent->notifyInputChanged(currentTimestamp, *engine, 1);
    }

    // incremental value
    auto tmp = engine->value(currentTimestamp, violationId);
    allDifferent->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->value(currentTimestamp, violationId));
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

  auto& invariant =
      engine->makeInvariant<MockAllDifferent>(viol, std::vector<VarId>{args});

  EXPECT_TRUE(invariant.initialized);

  EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(viol), numArgs - 1);
}

TEST_F(AllDifferentTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(AllDifferentTest, NotificationsOutputToInputNone) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(AllDifferentTest, NotificationsOutputToInputOutputToInputStatic) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(AllDifferentTest, NotificationsOutputToInputInputToOutputExploration) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
