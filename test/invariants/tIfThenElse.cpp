#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "variables/savedInt.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "invariants/ifThenElse.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockIfThenElse : public IfThenElse {
 public:
  bool m_initialized = false;

  void init(Timestamp timestamp, Engine& e) override {
    m_initialized = true;
    IfThenElse::init(timestamp, e);
  }

  MockIfThenElse(VarId b, VarId x, VarId y, VarId z)
      : IfThenElse(b, x, y, z) {
          ON_CALL(*this, recompute)
              .WillByDefault([this](Timestamp timestamp, Engine& engine) {
                return IfThenElse::recompute(timestamp, engine);
              });
          ON_CALL(*this, getNextDependency)
              .WillByDefault([this](Timestamp t, Engine& e) {
                return IfThenElse::getNextDependency(t, e);
              });

          ON_CALL(*this, notifyCurrentDependencyChanged)
              .WillByDefault([this](Timestamp t, Engine& e) {
                IfThenElse::notifyCurrentDependencyChanged(t, e);
              });

          ON_CALL(*this, notifyIntChanged)
              .WillByDefault([this](Timestamp t, Engine& e, LocalId id) {
                IfThenElse::notifyIntChanged(t, e, id);
              });

          ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& e) {
            IfThenElse::commit(t, e);
          });
  }

MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine), (override));

MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& e),
            (override));

MOCK_METHOD(void, notifyIntChanged, (Timestamp t, Engine& e, LocalId id),
            (override));
MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

private:
};

class IfThenElseTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode, bool doCommit) {
    engine->open();

    std::vector<VarId> args{};
    VarId b = engine->makeIntVar(0, -100, 100);
    VarId x = engine->makeIntVar(0, 0, 4);
    VarId y = engine->makeIntVar(5, 5, 9);
    VarId z = engine->makeIntVar(3, 0, 9);

    auto invariant = engine->makeInvariant<MockIfThenElse>(b, x, y, z);

    EXPECT_TRUE(invariant->m_initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->getPropagationMode() == PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else if (engine->getPropagationMode() == PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
      EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_)).Times(3);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    } else if (engine->getPropagationMode() == PropagationEngine::PropagationMode::MIXED) {
      EXPECT_EQ(0, 1);  // TODO: define the test case for mixed mode.
    }

    engine->beginMove();
    engine->setValue(b, 5);
    engine->endMove();

    if (doCommit) {
      engine->beginCommit();
      engine->query(z);
      engine->endCommit();
    } else {
      engine->beginQuery();
      engine->query(z);
      engine->endQuery();
    }
  }
};

TEST_F(IfThenElseTest, CreateElement) {
  engine->open();


  std::vector<VarId> args{};
  VarId b = engine->makeIntVar(0, -100, 100);
  VarId x = engine->makeIntVar(0, 0, 4);
  VarId y = engine->makeIntVar(5, 5, 9);
  VarId z = engine->makeIntVar(3, 0, 9);

  auto invariant = engine->makeInvariant<MockIfThenElse>(b, x, y, z);

  EXPECT_TRUE(invariant->m_initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(z), 0);
}

TEST_F(IfThenElseTest, NotificationsTopDown) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT, false);
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT, true);
}

TEST_F(IfThenElseTest, NotificationsBottomUp) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT, false);
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT, true);
}

}  // namespace
