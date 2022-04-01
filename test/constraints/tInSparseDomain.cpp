#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/inSparseDomain.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class MockInSparseDomain : public InSparseDomain {
 public:
  bool initialized = false;

  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    InSparseDomain::init(timestamp, engine);
  }

  MockInSparseDomain(VarId violationId, VarId x,
                     std::vector<DomainEntry> domain)
      : InSparseDomain(violationId, x, std::move(domain)) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return InSparseDomain::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return InSparseDomain::nextInput(t, engine);
        });

    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          InSparseDomain::notifyCurrentInputChanged(t, engine);
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          InSparseDomain::notifyInputChanged(t, engine, id);
        });

    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      InSparseDomain::commit(t, engine);
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

class InSparseDomainTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;
  VarId violationId = NULL_ID;
  VarId x = NULL_ID;
  std::vector<DomainEntry> domain;
  InSparseDomain* equal;
  std::mt19937 gen;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
    engine->open();
    x = engine->makeIntVar(2, -100, 100);
    domain = std::vector<DomainEntry>{{-100, -50}, {0, 50}, {100, 150}};
    violationId = engine->makeIntVar(0, 0, 200);

    equal = &(engine->makeConstraint<InSparseDomain>(violationId, x, domain));
    engine->close();
  }

  void testNotifications(PropagationMode propMode,
                         OutputToInputMarkingMode markingMode) {
    engine->open();

    const VarId a = engine->makeIntVar(5, -100, 100);
    const VarId viol = engine->makeIntVar(0, 0, 200);

    auto& invariant =
        engine->makeInvariant<MockInSparseDomain>(viol, a, domain);

    EXPECT_TRUE(invariant.initialized);

    EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

    EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

    engine->setPropagationMode(propMode);
    engine->setOutputToInputMarkingMode(markingMode);

    engine->close();

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(invariant, nextInput(testing::_, testing::_)).Times(0);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(0);
      EXPECT_CALL(invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(1);
    } else {
      EXPECT_CALL(invariant, nextInput(testing::_, testing::_)).Times(2);
      EXPECT_CALL(invariant, notifyCurrentInputChanged(testing::_, testing::_))
          .Times(1);

      EXPECT_CALL(invariant,
                  notifyInputChanged(testing::_, testing::_, testing::_))
          .Times(0);
    }

    engine->beginMove();
    engine->setValue(a, 0);
    engine->endMove();

    engine->beginProbe();
    engine->query(viol);
    engine->endProbe();
  }
};

/**
 *  Testing constructor
 */

TEST_F(InSparseDomainTest, Init) {
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(engine->tmpTimestamp(violationId), violationId), 0);
}

TEST_F(InSparseDomainTest, Recompute) {
  EXPECT_EQ(engine->value(0, violationId), 0);
  EXPECT_EQ(engine->committedValue(violationId), 0);

  Timestamp newTimestamp = 1;
  engine->setValue(newTimestamp, x, 60);
  equal->recompute(newTimestamp, *engine);
  EXPECT_EQ(engine->committedValue(violationId), 0);
  EXPECT_EQ(engine->value(newTimestamp, violationId), 10);
}

TEST_F(InSparseDomainTest, NotifyChange) {
  EXPECT_EQ(engine->value(0, violationId),
            0);  // initially the value of violationId is 0

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(engine->value(time1, x), 2);
  engine->setValue(time1, x, 60);
  EXPECT_EQ(engine->committedValue(x), 2);
  EXPECT_EQ(engine->value(time1, x), 60);
  equal->notifyInputChanged(time1, *engine, unused);
  EXPECT_EQ(engine->value(time1, violationId), 10);
}

TEST_F(InSparseDomainTest, Violation) {
  std::vector<DomainEntry> dom = {{-10, -5}, {-2, -2}, {0, 0}, {2, 2}, {5, 10}};

  Int lb = -200;
  Int ub = 200;

  std::vector<Int> values;
  for (Int val = lb; val <= ub; ++val) {
    values.emplace_back(val);
  }

  auto rng = std::default_random_engine{};
  std::shuffle(values.begin(), values.end(), rng);

  engine->open();

  VarId a = engine->makeIntVar(values.front(), lb, ub);
  VarId viol = engine->makeIntVar(0, 0, ub - lb + 1);

  engine->makeInvariant<InSparseDomain>(viol, a, std::vector<DomainEntry>{dom});

  engine->close();

  Int prevCommitted = engine->committedValue(viol);

  for (const Int val : values) {
    EXPECT_EQ(prevCommitted, engine->committedValue(viol));

    Int expectedViol = std::numeric_limits<Int>::max();
    for (const DomainEntry domEntry : dom) {
      if (val < domEntry.lowerBound) {
        expectedViol = std::min(expectedViol, domEntry.lowerBound - val);
      } else if (val <= domEntry.upperBound) {
        expectedViol = std::min(Int(0), expectedViol);
      } else {
        EXPECT_TRUE(val > domEntry.upperBound);
        expectedViol = std::min(expectedViol, val - domEntry.upperBound);
      }
    }

    engine->beginMove();
    engine->setValue(a, val);
    engine->endMove();

    engine->beginProbe();
    engine->query(viol);
    engine->endProbe();

    EXPECT_EQ(expectedViol, engine->currentValue(viol));

    engine->beginMove();
    engine->setValue(a, val);
    engine->endMove();

    engine->beginCommit();
    engine->query(viol);
    engine->endCommit();
    EXPECT_EQ(expectedViol, engine->committedValue(viol));

    prevCommitted = expectedViol;
  }
}

TEST_F(InSparseDomainTest, IncrementalVsRecompute) {
  EXPECT_EQ(engine->value(0, violationId),
            0);  // initially the value of violationId is 0
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTimestamp = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTimestamp;
    // Check that we do not accidentally commit
    ASSERT_EQ(engine->committedValue(x), 2);
    ASSERT_EQ(engine->committedValue(violationId),
              0);  // violationId is committed by register.

    // Set all variables
    engine->setValue(currentTimestamp, x, distribution(gen));

    // notify changes
    if (engine->committedValue(x) != engine->value(currentTimestamp, x)) {
      equal->notifyInputChanged(currentTimestamp, *engine, unused);
    }

    // incremental value
    auto tmp = engine->value(currentTimestamp, violationId);
    equal->recompute(currentTimestamp, *engine);

    ASSERT_EQ(tmp, engine->value(currentTimestamp, violationId));
  }
}

TEST_F(InSparseDomainTest, Commit) {
  EXPECT_EQ(engine->committedValue(violationId), 0);

  LocalId unused = -1;

  Timestamp currentTimestamp = 1;

  engine->setValue(currentTimestamp, x, 40);

  equal->notifyInputChanged(currentTimestamp, *engine, unused);

  // Committing an invariant does not commit its output!
  // // Commit at wrong timestamp should have no impact
  // equal->commit(currentTimestamp + 1, *engine);
  // EXPECT_EQ(engine->committedValue(violationId), 0);
  // equal->commit(currentTimestamp, *engine);
  // EXPECT_EQ(engine->committedValue(violationId), 38);
}

TEST_F(InSparseDomainTest, CreateInDomain) {
  engine->open();

  VarId a = engine->makeIntVar(100, -100, 100);

  VarId viol = engine->makeIntVar(0, 0, 200);

  auto& invariant = engine->makeInvariant<MockInSparseDomain>(
      viol, a, std::vector<DomainEntry>{{0, 0}});

  EXPECT_TRUE(invariant.initialized);

  EXPECT_CALL(invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  EXPECT_EQ(engine->currentValue(viol), 100);
}

TEST_F(InSparseDomainTest, NotificationsInputToOutput) {
  testNotifications(PropagationMode::INPUT_TO_OUTPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(InSparseDomainTest, NotificationsOutputToInputNone) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::NONE);
}

TEST_F(InSparseDomainTest, NotificationsOutputToInputOutputToInputStatic) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(InSparseDomainTest, NotificationsOutputToInputInputToOutputExploration) {
  testNotifications(PropagationMode::OUTPUT_TO_INPUT,
                    OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
