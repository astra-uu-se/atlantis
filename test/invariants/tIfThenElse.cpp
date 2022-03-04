
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/ifThenElse.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockIfThenElse : public IfThenElse {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    IfThenElse::init(timestamp, engine);
  }

  MockIfThenElse(VarId b, VarId x, VarId y, VarId z) : IfThenElse(b, x, y, z) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return IfThenElse::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp t, Engine& e) {
      return IfThenElse::nextInput(t, e);
    });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& e) {
          IfThenElse::notifyCurrentInputChanged(t, e);
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& e, LocalId id) {
          IfThenElse::notifyInputChanged(t, e, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& e) {
      IfThenElse::commit(t, e);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& e),
              (override));

  MOCK_METHOD(void, notifyInputChanged, (Timestamp t, Engine& e, LocalId id),
              (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class IfThenElseTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }

  void testNotifications(PropagationMode propMode,
                         OutputToInputMarkingMode markingMode) {
    engine->open();

    std::vector<VarId> args{};
    VarId b = engine->makeIntVar(0, -100, 100);
    VarId x = engine->makeIntVar(0, 0, 4);
    VarId y = engine->makeIntVar(5, 5, 9);
    VarId z = engine->makeIntVar(3, 0, 9);

    auto& invariant = engine->makeInvariant<MockIfThenElse>(b, x, y, z);

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
      EXPECT_CALL(invariant, nextInput(testing::_, testing::_)).Times(3);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    }

    engine->beginMove();
    engine->setValue(b, 5);
    engine->endMove();

    engine->beginProbe();
    engine->query(z);
    engine->endProbe();
  }
};

TEST_F(IfThenElseTest, CreateElement) {
  engine->open();

  std::vector<VarId> args{};
  VarId b = engine->makeIntVar(0, -100, 100);
  VarId x = engine->makeIntVar(0, 0, 4);
  VarId y = engine->makeIntVar(5, 5, 9);
  VarId z = engine->makeIntVar(3, 0, 9);

  auto& invariant = engine->makeInvariant<MockIfThenElse>(b, x, y, z);

  EXPECT_TRUE(invariant.initialized);

  EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(z), 0);
}

TEST_F(IfThenElseTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(IfThenElseTest, NotificationsOutputToInputNone) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(IfThenElseTest, NotificationsOutputToInputOutputToInputStatic) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(IfThenElseTest, NotificationsOutputToInputInputToOutputExploration) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
