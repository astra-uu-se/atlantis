#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

#include "constraints/globalCardinality.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "variables/committableInt.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

template <bool IsClosed>
class MockGlobalCardinality : public GlobalCardinality<IsClosed> {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    GlobalCardinality<IsClosed>::init(timestamp, engine);
  }

  MockGlobalCardinality(VarId violationId, std::vector<VarId>&& t_variables,
                        std::vector<Int>&& cover, std::vector<Int>&& t_counts)
      : GlobalCardinality<IsClosed>(
            violationId, std::vector<VarId>{t_variables},
            std::vector<Int>{cover}, std::vector<Int>{t_counts}) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return GlobalCardinality<IsClosed>::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return GlobalCardinality<IsClosed>::nextInput(timestamp, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          GlobalCardinality<IsClosed>::notifyCurrentInputChanged(timestamp,
                                                                 engine);
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault(
            [this](Timestamp timestamp, Engine& engine, LocalId localId) {
              GlobalCardinality<IsClosed>::notifyInputChanged(timestamp, engine,
                                                              localId);
            });

    ON_CALL(*this, commit)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          GlobalCardinality<IsClosed>::commit(timestamp, engine);
        });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp timestamp, Engine& engine, LocalId localId),
              (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

template <bool IsClosed>
class GlobalCardinalityTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  GlobalCardinality<IsClosed>* globalCardinality;
  std::mt19937 gen;

  void rapidCheckInternal(size_t, Int);

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    a = engine->makeIntVar(1, -100, 100);
    b = engine->makeIntVar(2, -100, 100);
    c = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 3);

    globalCardinality = &(engine->makeConstraint<GlobalCardinality<IsClosed>>(
        violationId, std::vector<VarId>({a, b, c}), std::vector<Int>({1, 2, 3}),
        std::vector<Int>({1, 1, 1})));
    engine->close();
  }

  void testNotifications(PropagationMode propMode,
                         OutputToInputMarkingMode markingMode) {
    engine->open();

    std::vector<VarId> args{};
    Int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(0, -100, 100));
    }

    VarId viol = engine->makeIntVar(0, 0, numArgs);

    auto invariant = &(engine->makeInvariant<MockGlobalCardinality<IsClosed>>(
        viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
        std::vector<Int>{1, 2, 3}));

    EXPECT_TRUE(invariant->initialized);

    EXPECT_CALL(*invariant, recompute(testing::_, testing::_))
        .Times(AtLeast(1));

    EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);
    engine->setOutputToInputMarkingMode(markingMode);

    engine->close();

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(AtMost(1));
      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(*invariant, nextInput(testing::_, testing::_))
          .Times(numArgs + 1);
      EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(*invariant,
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

class GlobalCardinalityTestClosed : public GlobalCardinalityTest<true> {};
class GlobalCardinalityTestOpen : public GlobalCardinalityTest<false> {};

/**
 *  Testing constructor
 */

TEST_F(GlobalCardinalityTestClosed, Init) {
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(violationId), violationId), 1);
}

TEST_F(GlobalCardinalityTestOpen, Init) {
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(violationId), violationId), 1);
}

TEST_F(GlobalCardinalityTestClosed, Recompute) {
  EXPECT_EQ(engine->value(0, violationId), 1);
  EXPECT_EQ(engine->committedValue(violationId), 1);

  Timestamp newTime = 1;

  engine->setValue(newTime, c, 3);
  globalCardinality->recompute(newTime, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(newTime, violationId), 0);

  engine->setValue(newTime, a, 2);
  globalCardinality->recompute(newTime, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(newTime, violationId), 1);
}

TEST_F(GlobalCardinalityTestOpen, Recompute) {
  EXPECT_EQ(engine->value(0, violationId), 1);
  EXPECT_EQ(engine->committedValue(violationId), 1);

  Timestamp newTime = 1;

  engine->setValue(newTime, c, 3);
  globalCardinality->recompute(newTime, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(newTime, violationId), 0);

  engine->setValue(newTime, a, 2);
  globalCardinality->recompute(newTime, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 1);
  EXPECT_EQ(engine->value(newTime, violationId), 1);
}

TEST_F(GlobalCardinalityTestClosed, NotifyChange) {
  EXPECT_EQ(engine->value(0, violationId), 1);

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, a), 1);
  engine->setValue(time1, a, 2);
  EXPECT_EQ(engine->committedValue(a), 1);
  EXPECT_EQ(engine->value(time1, a), 2);
  globalCardinality->notifyInputChanged(time1, *engine, 0);
  EXPECT_EQ(engine->value(time1, violationId), 2);

  engine->setValue(time1, b, 3);
  globalCardinality->notifyInputChanged(time1, *engine, 1);
  auto tmpValue = engine->value(time1, violationId);
  EXPECT_EQ(tmpValue, 1);

  // Incremental computation gives the same result as recomputation
  globalCardinality->recompute(time1, *engine);
  EXPECT_EQ(engine->value(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->value(time2, b), 2);
  engine->setValue(time2, b, 20);
  EXPECT_EQ(engine->committedValue(b), 2);
  EXPECT_EQ(engine->value(time2, b), 20);
  globalCardinality->notifyInputChanged(time2, *engine, 1);
  EXPECT_EQ(engine->value(time2, violationId), 1);
}

TEST_F(GlobalCardinalityTestOpen, NotifyChange) {
  EXPECT_EQ(engine->value(0, violationId), 1);

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, a), 1);
  engine->setValue(time1, a, 2);
  EXPECT_EQ(engine->committedValue(a), 1);
  EXPECT_EQ(engine->value(time1, a), 2);
  globalCardinality->notifyInputChanged(time1, *engine, 0);
  EXPECT_EQ(engine->value(time1, violationId), 2);

  engine->setValue(time1, b, 3);
  globalCardinality->notifyInputChanged(time1, *engine, 1);
  auto tmpValue = engine->value(time1, violationId);
  EXPECT_EQ(tmpValue, 1);

  // Incremental computation gives the same result as recomputation
  globalCardinality->recompute(time1, *engine);
  EXPECT_EQ(engine->value(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->value(time2, b), 2);
  engine->setValue(time2, b, 20);
  EXPECT_EQ(engine->committedValue(b), 2);
  EXPECT_EQ(engine->value(time2, b), 20);
  globalCardinality->notifyInputChanged(time2, *engine, 1);
  EXPECT_EQ(engine->value(time2, violationId), 1);
}

TEST_F(GlobalCardinalityTestClosed, IncrementalVsRecompute) {
  for (bool closed : std::vector<bool>{false, true}) {
    EXPECT_EQ(engine->value(0, violationId),
              1);  // initially the value of violationId is 0
    // todo: not clear if we actually want to deal with overflows...
    std::uniform_int_distribution<> distribution(closed ? 1 : -100,
                                                 closed ? 3 : 100);

    Timestamp currentTime = 1;
    for (size_t i = 0; i < 100; ++i) {
      ++currentTime;
      // Check that we do not accidentally commit
      ASSERT_EQ(engine->committedValue(a), 1);
      ASSERT_EQ(engine->committedValue(b), 2);
      ASSERT_EQ(engine->committedValue(violationId),
                1);  // violationId is commited by register.

      // Set all variables
      engine->setValue(currentTime, a, distribution(gen));
      engine->setValue(currentTime, b, distribution(gen));

      // notify changes
      if (engine->committedValue(a) != engine->value(currentTime, a)) {
        globalCardinality->notifyInputChanged(currentTime, *engine, 0);
      }
      if (engine->committedValue(b) != engine->value(currentTime, b)) {
        globalCardinality->notifyInputChanged(currentTime, *engine, 1);
      }

      // incremental value
      auto tmp = engine->value(currentTime, violationId);
      globalCardinality->recompute(currentTime, *engine);

      ASSERT_EQ(tmp, engine->value(currentTime, violationId));
    }
  }
}

TEST_F(GlobalCardinalityTestOpen, IncrementalVsRecompute) {
  for (bool closed : std::vector<bool>{false, true}) {
    EXPECT_EQ(engine->value(0, violationId),
              1);  // initially the value of violationId is 0
    // todo: not clear if we actually want to deal with overflows...
    std::uniform_int_distribution<> distribution(closed ? 1 : -100,
                                                 closed ? 3 : 100);

    Timestamp currentTime = 1;
    for (size_t i = 0; i < 100; ++i) {
      ++currentTime;
      // Check that we do not accidentally commit
      ASSERT_EQ(engine->committedValue(a), 1);
      ASSERT_EQ(engine->committedValue(b), 2);
      ASSERT_EQ(engine->committedValue(violationId),
                1);  // violationId is commited by register.

      // Set all variables
      engine->setValue(currentTime, a, distribution(gen));
      engine->setValue(currentTime, b, distribution(gen));

      // notify changes
      if (engine->committedValue(a) != engine->value(currentTime, a)) {
        globalCardinality->notifyInputChanged(currentTime, *engine, 0);
      }
      if (engine->committedValue(b) != engine->value(currentTime, b)) {
        globalCardinality->notifyInputChanged(currentTime, *engine, 1);
      }

      // incremental value
      auto tmp = engine->value(currentTime, violationId);
      globalCardinality->recompute(currentTime, *engine);

      ASSERT_EQ(tmp, engine->value(currentTime, violationId));
    }
  }
}

TEST_F(GlobalCardinalityTestClosed, Commit) {
  /*
  It is difficult to test the method commit as it only
  commits the internal data structures of the constraint.
  The internal data structures are (in almost all cases)
  private.
  */
  ASSERT_TRUE(true);
}

TEST_F(GlobalCardinalityTestOpen, Commit) {
  /*
  It is difficult to test the method commit as it only
  commits the internal data structures of the constraint.
  The internal data structures are (in almost all cases)
  private.
  */
  ASSERT_TRUE(true);
}

TEST_F(GlobalCardinalityTestClosed, CreateGlobalCardinality) {
  engine->open();

  std::vector<VarId> args{};
  Int numArgs = 10;
  for (Int value = 0; value < numArgs; ++value) {
    args.push_back(engine->makeIntVar(1, -100, 100));
  }

  VarId viol = engine->makeIntVar(0, 0, numArgs);

  auto invariant = &(engine->makeInvariant<MockGlobalCardinality<true>>(
      viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
      std::vector<Int>{1, 2, 3}));

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(viol), numArgs - 1);
}

TEST_F(GlobalCardinalityTestOpen, CreateGlobalCardinality) {
  engine->open();

  std::vector<VarId> args{};
  Int numArgs = 10;
  for (Int value = 0; value < numArgs; ++value) {
    args.push_back(engine->makeIntVar(1, -100, 100));
  }

  VarId viol = engine->makeIntVar(0, 0, numArgs);

  auto invariant = &(engine->makeInvariant<MockGlobalCardinality<true>>(
      viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
      std::vector<Int>{1, 2, 3}));

  EXPECT_TRUE(invariant->initialized);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(viol), numArgs - 1);
}

template <bool IsClosed>
void GlobalCardinalityTest<IsClosed>::rapidCheckInternal(size_t nVar,
                                                         Int valOffset) {
  size_t numVariables = static_cast<size_t>(nVar) + size_t(2);

  Int valLb = static_cast<Int>(valOffset - numVariables);
  Int valUb = static_cast<Int>(valOffset + numVariables);

  std::random_device rd;
  auto valDistribution = std::uniform_int_distribution<Int>{valLb, valUb};
  auto valGen = std::mt19937(rd());

  std::vector<Int> coverCounts(valUb - valLb + 1, 0);
  std::vector<VarId> variables;

  engine->open();
  for (size_t i = 0; i < numVariables; i++) {
    const Int val = valDistribution(valGen);
    coverCounts[val - valLb] += 1;
    variables.emplace_back(engine->makeIntVar(valLb, valLb, valUb));
  }

  std::vector<Int> cover;
  std::vector<Int> lowerBound;
  std::vector<Int> upperBound;
  for (size_t i = 0; i < coverCounts.size(); ++i) {
    if (coverCounts[i] > 0) {
      cover.emplace_back(i + valLb);
      lowerBound.emplace_back(std::max(Int(0), coverCounts[i] - 1));
      upperBound.emplace_back(std::max(Int(0), coverCounts[i]));
    }
  }

  VarId viol = engine->makeIntVar(0, 0, static_cast<Int>(numVariables));

  engine->makeInvariant<GlobalCardinality<IsClosed>>(viol, variables, cover,
                                                     lowerBound, upperBound);

  engine->close();

  // There are currently a bug in PropagationEngine that is resolved in another
  // branch.
  for (auto [propMode, markMode] :
       std::vector<std::pair<PropagationMode, OutputToInputMarkingMode>>{
           {PropagationMode::INPUT_TO_OUTPUT,
            OutputToInputMarkingMode::NONE}  //,
           //{PropagationMode::OUTPUT_TO_INPUT, OutputToInputMarkingMode::NONE},
           //{PropagationMode::OUTPUT_TO_INPUT,
           // OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION},
           //{PropagationMode::OUTPUT_TO_INPUT,
           // OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC}
       }) {
    for (size_t iter = 0; iter < 3; ++iter) {
      engine->open();
      engine->setPropagationMode(propMode);
      engine->setOutputToInputMarkingMode(markMode);
      engine->close();

      engine->beginMove();
      for (const VarId x : variables) {
        engine->setValue(x, valDistribution(valGen));
      }
      engine->endMove();

      engine->beginProbe();
      engine->query(viol);
      engine->endProbe();

      std::unordered_map<Int, std::pair<Int, Int>> bounds;
      std::unordered_map<Int, Int> actualCounts;
      Int outsideCount = 0;

      for (size_t i = 0; i < cover.size(); ++i) {
        bounds.emplace(cover[i], std::pair(lowerBound[i], upperBound[i]));
        actualCounts.emplace(cover[i], 0);
      }

      for (const VarId varId : variables) {
        Int val = engine->currentValue(varId);
        if (actualCounts.count(val) <= 0) {
          ++outsideCount;
        } else {
          ++actualCounts[val];
        }
      }

      Int shortage = 0;
      Int excess = 0;
      if constexpr (IsClosed) {
        excess = outsideCount;
      }

      for (const Int val : cover) {
        RC_ASSERT(actualCounts.count(val) > size_t(0) &&
                  bounds.count(val) > size_t(0));
        const auto [l, u] = bounds.at(val);
        const auto actual = actualCounts.at(val);
        shortage += std::max(Int(0), l - actual);
        excess += std::max(Int(0), actual - u);
      }

      Int actualViolation = engine->currentValue(viol);
      Int expectedViolation = std::max(shortage, excess);
      if (actualViolation != expectedViolation) {
        RC_ASSERT(actualViolation == expectedViolation);
      }
    }
  }
}

RC_GTEST_FIXTURE_PROP(GlobalCardinalityTestOpen, rapidCheck,
                      (unsigned char nVar, int valOffset)) {
  rapidCheckInternal(static_cast<size_t>(nVar), static_cast<Int>(valOffset));
}

RC_GTEST_FIXTURE_PROP(GlobalCardinalityTestClosed, rapidCheck,
                      (unsigned char nVar, int valOffset)) {
  rapidCheckInternal(static_cast<size_t>(nVar), static_cast<Int>(valOffset));
}

TEST_F(GlobalCardinalityTestClosed, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(GlobalCardinalityTestClosed, NotificationsOutputToInputNone) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(GlobalCardinalityTestClosed,
       NotificationsOutputToInputOutputToInputStatic) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(GlobalCardinalityTestClosed,
       NotificationsOutputToInputInputToOutputExploration) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
