#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "invariants/absDiff.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class MockAbsDiff : public AbsDiff {
 public:
  MockAbsDiff(VarId a, VarId b, VarId c) : AbsDiff(a, b, c), real_(a, b, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return real_.recompute(timestamp, engine);
        });
    // ON_CALL(*this, getNextDependency)
    //     .WillByDefault([this](Timestamp t, Engine& e) {
    //       return real_.getNextDependency(t, e);
    //     });

    // ON_CALL(*this, notifyCurrentDependencyChanged)
    //     .WillByDefault([this](Timestamp t, Engine& e) {
    //       real_.notifyCurrentDependencyChanged(t, e);
    //     });

    // ON_CALL(*this, notifyIntChanged)
    //     .WillByDefault([this](Timestamp t, Engine& e, LocalId id, Int
    //     oldValue,
    //                           Int newValue, Int data) {
    //       real_.notifyIntChanged(t, e, id, oldValue, newValue, data);
    //     });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& e) {
      real_.commit(t, e);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  // MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
  // MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& e),
  //             (override));

  // MOCK_METHOD(void, notifyIntChanged,
  //             (Timestamp t, Engine& e, LocalId id, Int oldValue, Int
  //             newValue,
  //              Int data),
  //             (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
  AbsDiff real_;
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

}  // namespace
