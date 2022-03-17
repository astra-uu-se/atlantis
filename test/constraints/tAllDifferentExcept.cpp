#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/allDifferentExcept.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class MockAllDifferentExcept : public AllDifferentExcept {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    AllDifferentExcept::init(timestamp, engine);
  }

  MockAllDifferentExcept(VarId violationId, std::vector<VarId>&& t_variables,
                         std::vector<Int>&& t_ignored)
      : AllDifferentExcept(violationId, std::vector<VarId>{t_variables},
                           std::vector<Int>{t_ignored}) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return AllDifferentExcept::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return AllDifferentExcept::nextInput(t, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          AllDifferentExcept::notifyCurrentInputChanged(t, engine);
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          AllDifferentExcept::notifyInputChanged(t, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      AllDifferentExcept::commit(t, engine);
    });
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));

  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp t, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));

 private:
};

class AllDifferentExceptTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  AllDifferentExcept* allDifferentExcept;
  std::mt19937 gen;

  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    a = engine->makeIntVar(1, -100, 100);
    b = engine->makeIntVar(2, -100, 100);
    c = engine->makeIntVar(2, -100, 100);
    violationId = engine->makeIntVar(0, 0, 3);

    allDifferentExcept = &(engine->makeConstraint<AllDifferentExcept>(
        violationId, std::vector<VarId>{a, b, c}, std::vector<Int>{0, 2}));
    engine->close();
  }

  void testNotifications(PropagationMode propMode,
                         OutputToInputMarkingMode markingMode) {
    engine->open();

    std::vector<VarId> args{};
    int numArgs = 10;
    args.reserve(numArgs);
    for (int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(0, -100, 100));
    }

    VarId viol = engine->makeIntVar(0, 0, numArgs);

    auto& invariant = engine->makeInvariant<MockAllDifferentExcept>(
        viol, std::vector<VarId>{args}, std::vector<Int>{0, 1, 100});

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
      EXPECT_CALL(invariant, nextInput(testing::_, testing::_))
          .Times(numArgs + 1);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(invariant,
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

/**
 *  Testing constructor
 */

TEST_F(AllDifferentExceptTest, Init) {
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(violationId), violationId), 0);
}

TEST_F(AllDifferentExceptTest, Recompute) {
  EXPECT_EQ(engine->value(0, violationId), 0);
  EXPECT_EQ(engine->committedValue(violationId), 0);

  Timestamp newTimestamp = 1;

  engine->setValue(newTimestamp, c, 1);
  allDifferentExcept->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(newTimestamp, violationId), 1);

  engine->setValue(newTimestamp, b, 1);
  allDifferentExcept->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(newTimestamp, violationId), 2);
}

TEST_F(AllDifferentExceptTest, NotifyChange) {
  EXPECT_EQ(engine->value(0, violationId), 0);

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, a), 1);
  engine->setValue(time1, a, 2);
  EXPECT_EQ(engine->committedValue(a), 1);
  EXPECT_EQ(engine->value(time1, a), 2);
  allDifferentExcept->notifyInputChanged(time1, *engine, 0);
  EXPECT_EQ(engine->value(time1, violationId), 0);

  engine->setValue(time1, b, 3);
  allDifferentExcept->notifyInputChanged(time1, *engine, 1);
  auto tmpValue = engine->value(time1, violationId);

  // Incremental computation gives the same result as recomputation
  allDifferentExcept->recompute(time1, *engine);
  EXPECT_EQ(engine->value(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(engine->value(time2, b), 2);
  engine->setValue(time2, b, 20);
  EXPECT_EQ(engine->committedValue(b), 2);
  EXPECT_EQ(engine->value(time2, b), 20);
  allDifferentExcept->notifyInputChanged(time2, *engine, 1);
  EXPECT_EQ(engine->value(time2, violationId), 0);
}

TEST_F(AllDifferentExceptTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->value(0, violationId),
            0);  // initially the value of violationId is 0
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100, 100);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 2; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit:
    ASSERT_EQ(engine->committedValue(a), 1);
    ASSERT_EQ(engine->committedValue(b), 2);
    ASSERT_EQ(engine->committedValue(c), 2);
    // violationId is commited by register:
    ASSERT_EQ(engine->committedValue(violationId), 0);

    // Set all variables
    engine->setValue(currentTimestamp, a, distribution(gen));
    engine->setValue(currentTimestamp, b, distribution(gen));
    engine->setValue(currentTimestamp, c, distribution(gen));

    // notify changes
    if (engine->committedValue(a) != engine->value(currentTimestamp, a)) {
      allDifferentExcept->notifyInputChanged(currentTimestamp, *engine, 0);
    }
    if (engine->committedValue(b) != engine->value(currentTimestamp, b)) {
      allDifferentExcept->notifyInputChanged(currentTimestamp, *engine, 1);
    }
    if (engine->committedValue(c) != engine->value(currentTimestamp, c)) {
      allDifferentExcept->notifyInputChanged(currentTimestamp, *engine, 2);
    }

    // incremental value
    auto tmp = engine->value(currentTimestamp, violationId);
    allDifferentExcept->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->value(currentTimestamp, violationId));
  }
}

TEST_F(AllDifferentExceptTest, Commit) {
  /*
  It is difficult to test the method commit as it only
  commits the internal data structures of the constraint.
  The internal data structures are (in almost all cases)
  private.
  */
  ASSERT_TRUE(true);
}

TEST_F(AllDifferentExceptTest, CreateAllDifferentExcept) {
  engine->open();

  std::vector<VarId> args{};
  Int numArgs = 10;
  for (Int value = 0; value < numArgs; ++value) {
    args.push_back(engine->makeIntVar(0, -100, 100));
  }

  VarId viol = engine->makeIntVar(0, 0, numArgs);

  auto& invariant = engine->makeInvariant<MockAllDifferentExcept>(
      viol, std::vector<VarId>{args}, std::vector<Int>{10, 100});

  EXPECT_TRUE(invariant.initialized);

  EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(viol), numArgs - 1);
}

RC_GTEST_FIXTURE_PROP(AllDifferentExceptTest, rapidCheck,
                      (unsigned char nVar, int valOffset)) {
  size_t numVariables = static_cast<size_t>(nVar) + size_t(2);

  Int lb = static_cast<Int>(valOffset - numVariables);
  Int ub = static_cast<Int>(valOffset + numVariables);

  std::random_device rd;
  auto valDistribution = std::uniform_int_distribution<Int>{lb, ub};
  auto valGen = std::mt19937(rd());

  std::unordered_set<Int> ignored;
  std::vector<VarId> variables;

  engine->open();
  for (size_t i = 0; i < numVariables; i++) {
    ignored.emplace(valDistribution(valGen));
    variables.emplace_back(engine->makeIntVar(valDistribution(valGen), lb, ub));
  }

  std::vector<Int> ignoredVec;
  std::vector<Int> ignoredVecHr;
  for (Int ignoredVal : ignored) {
    ignoredVec.emplace_back(ignoredVal);
    ignoredVecHr.emplace_back(ignoredVal - lb);
  }

  VarId viol = engine->makeIntVar(0, 0, static_cast<Int>(numVariables));

  engine->makeInvariant<AllDifferentExcept>(viol, variables, ignoredVec);

  engine->close();

  for (auto [propMode, markMode] :
       std::vector<std::pair<PropagationMode, OutputToInputMarkingMode>>{
           {PropagationMode::INPUT_TO_OUTPUT, OutputToInputMarkingMode::NONE},
           {PropagationMode::OUTPUT_TO_INPUT, OutputToInputMarkingMode::NONE},
           {PropagationMode::OUTPUT_TO_INPUT,
            OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION},
           {PropagationMode::OUTPUT_TO_INPUT,
            OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC}}) {
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

      Int expectedViolation = 0;

      std::vector<bool> checkedVars(variables.size(), false);
      std::vector<Int> valuesHr;

      for (size_t i = 0; i < variables.size(); ++i) {
        const Int iVal = engine->currentValue(variables[i]);
        valuesHr.emplace_back(iVal - lb);
        if (checkedVars[i] || ignored.count(iVal) > 0) {
          continue;
        }
        for (size_t j = i + 1; j < variables.size(); ++j) {
          const Int jVal = engine->currentValue(variables[j]);
          if (checkedVars[j] || ignored.count(jVal) > 0) {
            continue;
          }
          if (iVal == jVal) {
            checkedVars[j] = true;
            ++expectedViolation;
          }
        }
      }
      Int actualViolation = engine->currentValue(viol);
      if (actualViolation != expectedViolation) {
        RC_ASSERT(actualViolation == expectedViolation);
      }
    }
  }
}

TEST_F(AllDifferentExceptTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(AllDifferentExceptTest, NotificationsOutputToInputNone) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(AllDifferentExceptTest, NotificationsOutputToInputOutputToInputStatic) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(AllDifferentExceptTest,
       NotificationsOutputToInputInputToOutputExploration) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
