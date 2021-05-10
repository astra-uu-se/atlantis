#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "variables/savedInt.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "invariants/absDiff.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;
using ::testing::Return;

namespace {

class MockAbsDiff : public AbsDiff {
 public:
  bool m_initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    m_initialized = true;
    AbsDiff::init(timestamp, engine);
  }

  MockAbsDiff(VarId a, VarId b, VarId c) : AbsDiff(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          AbsDiff::recompute(timestamp, engine);
        });
    ON_CALL(*this, getNextDependency)
        .WillByDefault([this](Timestamp t, Engine& e) {
          return AbsDiff::getNextDependency(t, e);
        });

    ON_CALL(*this, notifyCurrentDependencyChanged)
        .WillByDefault([this](Timestamp t, Engine& e) {
          AbsDiff::notifyCurrentDependencyChanged(t, e);
        });
    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp t, Engine& e, LocalId id) {
          AbsDiff::notifyIntChanged(t, e, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& e) {
      AbsDiff::commit(t, e);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));
  MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& e),
              (override));

  MOCK_METHOD(void, notifyIntChanged, (Timestamp t, Engine& e, LocalId id),
              (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
};

class AbsDiffTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }

  void testNotifications(PropagationEngine::PropagationMode propMode) {
    engine->open();

    VarId a = engine->makeIntVar(-10, -100, 100);
    VarId b = engine->makeIntVar(10, -100, 100);

    VarId output = engine->makeIntVar(0, 0, 200);

    auto invariant = engine->makeInvariant<MockAbsDiff>(a, b, output);

    EXPECT_TRUE(invariant->m_initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->mode = propMode;

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
    } else if (engine->mode == PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
      EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_))
          .Times(3);
      EXPECT_CALL(*invariant,
                  notifyCurrentDependencyChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    } else if (engine->mode == PropagationEngine::PropagationMode::MIXED) {
      EXPECT_EQ(0, 1);  // TODO: define the test case.
    }

    engine->beginMove();
    engine->setValue(a, 0);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();
  }
};

TEST_F(AbsDiffTest, CreateAbsDiff) {
  engine->open();

  auto a = engine->makeIntVar(100, -100, 100);
  auto b = engine->makeIntVar(-100, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = engine->makeInvariant<MockAbsDiff>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->getNewValue(c), 200);
}

TEST_F(AbsDiffTest, Modification) {
  engine->open();

  auto a = engine->makeIntVar(100, -100, 100);
  auto b = engine->makeIntVar(-100, -100, 100);
  auto c = engine->makeIntVar(0, 0, 200);

  auto invariant = engine->makeInvariant<MockAbsDiff>(a, b, c);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  if (engine->mode == PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant,
                notifyIntChanged(testing::_, testing::_, testing::_))
        .Times(AtLeast(1));
    EXPECT_CALL(*invariant,
                notifyCurrentDependencyChanged(testing::_, testing::_))
        .Times(AnyNumber());
  } else if (engine->mode == PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
    EXPECT_CALL(*invariant, getNextDependency(testing::_, testing::_))
        .Times(AtLeast(2));
    EXPECT_CALL(*invariant,
                notifyCurrentDependencyChanged(testing::_, testing::_))
        .Times(Exactly(1));
  }

  engine->close();

  EXPECT_EQ(engine->getNewValue(c), 200);

  engine->beginMove();
  engine->setValue(a, 0);
  engine->endMove();

  engine->beginQuery();
  engine->query(c);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(c), 100);
}

TEST_F(AbsDiffTest, NotificationsTopDown) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(AbsDiffTest, NotificationsBottomUp) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
