#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/intDiv.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class MockIntDiv : public IntDiv {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    IntDiv::init(timestamp, engine);
  }

  MockIntDiv(VarId a, VarId b, VarId c) : IntDiv(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          IntDiv::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return IntDiv::nextInput(ts, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          IntDiv::notifyCurrentInputChanged(ts, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          IntDiv::notifyInputChanged(ts, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      IntDiv::commit(ts, engine);
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
};

class IntDivTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    engine = std::make_unique<PropagationEngine>();
  }

  void testNotifications(PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(-10, -100, 100);
    VarId b = engine->makeIntVar(10, -100, 100);

    VarId output = engine->makeIntVar(0, 0, 200);

    auto invariant = &engine->makeInvariant<MockIntDiv>(a, b, output);

    EXPECT_TRUE(invariant->initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->propagationMode() ==
        PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(3);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(a, 0);
    engine->endMove();

    engine->beginProbe();
    engine->query(output);
    engine->endProbe();
  }
};

TEST_F(IntDivTest, CreateIntDiv) {
  engine->open();

  auto a = engine->makeIntVar(10, -100, 100);
  auto b = engine->makeIntVar(5, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = &engine->makeInvariant<MockIntDiv>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(c), 2);
}

TEST_F(IntDivTest, Modification) {
  engine->open();

  auto a = engine->makeIntVar(10, -100, 100);
  auto b = engine->makeIntVar(2, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = &engine->makeInvariant<MockIntDiv>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  if (engine->propagationMode() ==
      PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant,
                notifyInputChanged(testing::_, testing::_, testing::_))
        .Times(AtLeast(1));
    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(AnyNumber());
  } else if (engine->propagationMode() ==
             PropagationMode::OUTPUT_TO_INPUT) {
    EXPECT_CALL(*invariant, nextInput(testing::_, testing::_))
        .Times(AtLeast(2));
    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(Exactly(1));
  }

  engine->close();

  EXPECT_EQ(engine->currentValue(c), 5);

  engine->beginMove();
  engine->setValue(a, 8);
  engine->endMove();

  engine->beginProbe();
  engine->query(c);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(c), 4);
}

TEST_F(IntDivTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(IntDivTest, NotificationsOutputToInput) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
