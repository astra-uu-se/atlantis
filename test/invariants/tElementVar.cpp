
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "invariants/elementVar.hpp"
#include "variables/savedInt.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockElementVar : public ElementVar {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    ElementVar::init(timestamp, engine);
  }

  MockElementVar(VarId i, std::vector<VarId>&& X, VarId b)
      : ElementVar(i, std::vector<VarId>{X}, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return ElementVar::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextParameter)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return ElementVar::nextParameter(ts, engine);
        });

    ON_CALL(*this, notifyCurrentParameterChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          ElementVar::notifyCurrentParameterChanged(ts, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          ElementVar::notifyIntChanged(ts, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      ElementVar::commit(ts, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, nextParameter, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentParameterChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyIntChanged,
              (Timestamp ts, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class ElementVarTest : public ::testing::Test {
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

    std::vector<VarId> args{};
    int numArgs = 10;
    for (int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(value, -100, 100));
    }

    VarId idx = engine->makeIntVar(0, 0, numArgs - 1);

    VarId output = engine->makeIntVar(-10, -100, 100);

    auto invariant = engine->makeInvariant<MockElementVar>(
        idx, std::vector<VarId>{args}, output);

    EXPECT_TRUE(invariant->initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);

    engine->close();

    if (engine->mode == PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextParameter(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant,
                  notifyCurrentParameterChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else if (engine->mode ==
               PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
      EXPECT_CALL(*invariant, nextParameter(testing::_, testing::_)).Times(3);
      EXPECT_CALL(*invariant,
                  notifyCurrentParameterChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, testing::_))
          .Times(AtMost(1));
    } else if (engine->mode == PropagationEngine::PropagationMode::MIXED) {
      EXPECT_EQ(0, 1);  // TODO: define the test case for mixed mode.
    }

    engine->beginMove();
    engine->setValue(idx, 5);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();
  }
};

TEST_F(ElementVarTest, CreateElement) {
  engine->open();

  std::vector<VarId> args{};
  int numArgs = 10;
  for (int value = 0; value < numArgs; ++value) {
    args.push_back(engine->makeIntVar(value, -100, 100));
  }

  VarId idx = engine->makeIntVar(3, 0, numArgs - 1);

  VarId output = engine->makeIntVar(-10, -100, 100);

  auto invariant = engine->makeInvariant<MockElementVar>(
      idx, std::vector<VarId>{args}, output);

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->newValue(output), 3);
}

TEST_F(ElementVarTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(ElementVarTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
