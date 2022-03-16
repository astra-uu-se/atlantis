#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/mod.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class MockMod : public Mod {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    Mod::init(timestamp, engine);
  }

  MockMod(VarId a, VarId b, VarId c) : Mod(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          Mod::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return Mod::nextInput(ts, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          Mod::notifyCurrentInputChanged(ts, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          Mod::notifyInputChanged(ts, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      Mod::commit(ts, engine);
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

class ModTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override {
    engine = std::make_unique<PropagationEngine>();
  }

  void testNotifications(PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(-10, -100, 100);
    VarId b = engine->makeIntVar(10, -100, 100);

    VarId output = engine->makeIntVar(0, 0, 200);

    auto invariant = &engine->makeInvariant<MockMod>(a, b, output);

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

TEST_F(ModTest, CreateMod) {
  engine->open();

  auto a = engine->makeIntVar(4, -100, 100);
  auto b = engine->makeIntVar(2, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = &engine->makeInvariant<MockMod>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(c), 0);
}

TEST_F(ModTest, Modification) {
  engine->open();

  auto a = engine->makeIntVar(5, -100, 100);
  auto b = engine->makeIntVar(2, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = &engine->makeInvariant<MockMod>(a, b, c);

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

  EXPECT_EQ(engine->currentValue(c), 1);

  engine->beginMove();
  engine->setValue(a, 6);
  engine->endMove();

  engine->beginProbe();
  engine->query(c);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(c), 0);
}

TEST_F(ModTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(ModTest, NotificationsOutputToInput) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
