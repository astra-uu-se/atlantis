
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "invariants/elementConst.hpp"
#include "variables/savedInt.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockElementConst : public ElementConst {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    ElementConst::init(timestamp, engine);
  }

  MockElementConst(VarId i, std::vector<Int>&& X, VarId b)
      : ElementConst(i, std::vector<Int>{X}, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return ElementConst::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextParameter)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          return ElementConst::nextParameter(ts, engine);
        });

    ON_CALL(*this, notifyCurrentParameterChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          ElementConst::notifyCurrentParameterChanged(ts, engine);
        });

    ON_CALL(*this, notifyIntChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId id) {
          ElementConst::notifyIntChanged(ts, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp ts, Engine& engine) {
      ElementConst::commit(ts, engine);
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

class ElementConstTest : public ::testing::Test {
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

    std::vector<Int> args{};
    int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(value);
    }

    VarId idx = engine->makeIntVar(0, 0, numArgs - 1);

    VarId output = engine->makeIntVar(-10, -100, 100);

    auto invariant = engine->makeInvariant<MockElementConst>(
        idx, std::vector<Int>{args}, output);

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
      EXPECT_CALL(*invariant, nextParameter(testing::_, testing::_)).Times(2);
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

TEST_F(ElementConstTest, CreateElement) {
  engine->open();

  std::vector<Int> args{};
  int numArgs = 10;
  for (Int value = 0; value < numArgs; ++value) {
    args.push_back(value);
  }

  VarId idx = engine->makeIntVar(3, 0, numArgs - 1);

  VarId output = engine->makeIntVar(-10, -100, 100);

  auto invariant = engine->makeInvariant<MockElementConst>(
      idx, std::vector<Int>{args}, output);

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->newValue(output), 3);
}

TEST_F(ElementConstTest, NotificationsInputToOutput) {
  testNotifications(PropagationEngine::PropagationMode::INPUT_TO_OUTPUT);
}

TEST_F(ElementConstTest, NotificationsOutputToInput) {
  testNotifications(PropagationEngine::PropagationMode::OUTPUT_TO_INPUT);
}

}  // namespace
