#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/times.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class MockTimes : public Times {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    Times::init(timestamp, engine);
  }

  MockTimes(VarId a, VarId b, VarId c) : Times(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          Times::recompute(timestamp, engine);
        });
    ON_CALL(*this, getNextInput)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return Times::getNextInput(ts, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          Times::notifyCurrentInputChanged(ts, engine);
        });
    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          Times::notifyIntChanged(ts, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      Times::commit(ts, engine);
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
};

class TimesTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    engine = std::make_unique<PropagationEngine>();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(-10, -100, 100);
    VarId b = engine->makeIntVar(10, -100, 100);

    VarId output = engine->makeIntVar(0, 0, 200);

    auto invariant = engine->makeInvariant<MockTimes>(a, b, output);

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
      EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_)).Times(3);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(a, 0);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();
  }
};

TEST_F(TimesTest, CreateTimes) {
  engine->open();

  auto a = engine->makeIntVar(10, -100, 100);
  auto b = engine->makeIntVar(5, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = engine->makeInvariant<MockTimes>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(c), 50);
}

TEST_F(TimesTest, Modification) {
  engine->open();

  auto a = engine->makeIntVar(10, -100, 100);
  auto b = engine->makeIntVar(2, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = engine->makeInvariant<MockTimes>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  if (engine->propagationMode ==
      PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant,
                notifyIntChanged(testing::_, testing::_, testing::_))
        .Times(AtLeast(1));
    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(AnyNumber());
  } else if (engine->propagationMode ==
             PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
    EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_))
        .Times(AtLeast(2));
    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(Exactly(1));
  }

  engine->close();

  EXPECT_EQ(engine->getNewValue(c), 20);

  engine->beginMove();
  engine->setValue(a, 8);
  engine->endMove();

  engine->beginQuery();
  engine->query(c);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(c), 16);
}

TEST_F(TimesTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(TimesTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

TEST_F(TimesTest, NotificationsMixed) {
  testNotifications(PropagationEngine::PropagationMode::MIXED);
}

}  // namespace
