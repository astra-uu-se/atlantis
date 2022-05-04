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

class InSparseDomainTest : public InvariantTest {
 public:
  Int computeViolation(Timestamp ts, VarId var,
                       const std::vector<DomainEntry>& domain) {
    return computeViolation(engine->value(ts, var), domain);
  }
  Int computeViolation(Int val, const std::vector<DomainEntry>& domain) {
    Int viol = std::numeric_limits<Int>::max();
    for (const auto& [lb, ub] : domain) {
      if (lb <= val && val <= ub) {
        viol = 0;
        break;
      } else if (val < lb) {
        viol = std::min(viol, lb - val);
      } else if (ub < val) {
        viol = std::min(viol, val - ub);
      }
    }
    return viol;
  }
};

TEST_F(InSparseDomainTest, UpdateBounds) {
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  engine->open();
  const VarId x = engine->makeIntVar(domainVec.front().lowerBound,
                                     domainVec.front().lowerBound,
                                     domainVec.front().upperBound);

  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    for (const auto& [xLb, xUb] : domainVec) {
      EXPECT_TRUE(xLb <= xUb);
      if (!engine->isOpen()) {
        engine->open();
      }
      engine->updateBounds(x, xLb, xUb, false);
      const VarId violationId = engine->makeIntVar(0, 0, 2);
      InSparseDomain& invariant = engine->makeConstraint<InSparseDomain>(
          violationId, x, std::vector<DomainEntry>(dom));
      engine->close();

      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        engine->setValue(engine->currentTimestamp(), x, xVal);
        invariant.recompute(engine->currentTimestamp(), *engine);
        violations.emplace_back(
            engine->value(engine->currentTimestamp(), violationId));
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_EQ(*minViol, engine->lowerBound(violationId));
      ASSERT_TRUE(*maxViol <= engine->upperBound(violationId));
    }
  }
}

TEST_F(InSparseDomainTest, Recompute) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!engine->isOpen()) {
      engine->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;
    const VarId x = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntVar(0, 0, lb + ub);
    InSparseDomain& invariant = engine->makeConstraint<InSparseDomain>(
        violationId, x, std::vector<DomainEntry>(dom));
    engine->close();
    for (Int val = lb; val <= ub; ++val) {
      engine->setValue(engine->currentTimestamp(), x, val);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(computeViolation(val, dom),
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(InSparseDomainTest, NotifyInputChanged) {
  const Int margin = 20;
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!engine->isOpen()) {
      engine->open();
    }
    const Int lb = domainVec.front().lowerBound - margin;
    const Int ub = domainVec.back().upperBound + margin;
    const VarId x = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntVar(0, 0, lb + ub);
    InSparseDomain& invariant = engine->makeConstraint<InSparseDomain>(
        violationId, x, std::vector<DomainEntry>(dom));
    engine->close();
    for (Int val = lb; val <= ub; ++val) {
      engine->setValue(engine->currentTimestamp(), x, val);
      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(0));
      EXPECT_EQ(computeViolation(val, dom),
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(InSparseDomainTest, NextInput) {
  const Int margin = 20;
  std::vector<DomainEntry> dom{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  const Int lb = dom.front().lowerBound - margin;
  const Int ub = dom.back().upperBound + margin;
  engine->open();
  const VarId x = engine->makeIntVar(lb, lb, ub);
  const VarId violationId = engine->makeIntVar(0, 0, lb + ub);
  InSparseDomain& invariant = engine->makeConstraint<InSparseDomain>(
      violationId, x, std::vector<DomainEntry>(dom));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    EXPECT_EQ(invariant.nextInput(ts, *engine), x);
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
  }
}

TEST_F(InSparseDomainTest, NotifyCurrentInputChanged) {
  const Int margin = 20;
  std::vector<DomainEntry> dom{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};
  const Int lb = dom.front().lowerBound - margin;
  const Int ub = dom.back().upperBound + margin;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  engine->open();

  const VarId x = engine->makeIntVar(lb, lb, ub);
  const VarId violationId = engine->makeIntVar(0, 0, lb + ub);
  InSparseDomain& invariant = engine->makeConstraint<InSparseDomain>(
      violationId, x, std::vector<DomainEntry>(dom));
  engine->close();
  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    EXPECT_EQ(invariant.nextInput(ts, *engine), x);
    const Int oldVal = engine->value(ts, x);
    do {
      engine->setValue(ts, x, valueDist(gen));
    } while (engine->value(ts, x) == oldVal);
    invariant.notifyCurrentInputChanged(ts, *engine);
    EXPECT_EQ(engine->value(ts, violationId), computeViolation(ts, x, dom));
  }
}

TEST_F(InSparseDomainTest, Commit) {
  std::vector<DomainEntry> domainVec{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};

  const Int margin = 20;
  const Int lb = domainVec.front().lowerBound - margin;
  const Int ub = domainVec.back().upperBound + margin;
  std::uniform_int_distribution<Int> valueDist(lb, ub);

  for (const std::vector<DomainEntry>& dom : subsets(domainVec)) {
    if (dom.size() == 0) {
      continue;
    }
    if (!engine->isOpen()) {
      engine->open();
    }

    const VarId x = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntVar(0, 0, lb + ub);
    InSparseDomain& invariant = engine->makeConstraint<InSparseDomain>(
        violationId, x, std::vector<DomainEntry>(dom));
    engine->close();

    Int committedValue = engine->committedValue(x);

    for (size_t i = 0; i < 4; ++i) {
      EXPECT_EQ(engine->value(engine->currentTimestamp(), violationId),
                computeViolation(engine->currentTimestamp(), x, dom));

      Timestamp ts = engine->currentTimestamp() + Timestamp(1 + i);

      // Check that we do not accidentally commit:
      ASSERT_EQ(engine->committedValue(x), committedValue);

      const Int oldVal = committedValue;
      do {
        engine->setValue(ts, x, valueDist(gen));
      } while (oldVal == engine->value(ts, x));

      // notify changes
      invariant.notifyInputChanged(ts, *engine, LocalId(0));

      // incremental value
      const Int notifiedViolation = engine->value(ts, violationId);
      invariant.recompute(ts, *engine);

      ASSERT_EQ(notifiedViolation, engine->value(ts, violationId));

      engine->commitIf(ts, x);
      committedValue = engine->value(ts, x);
      engine->commitIf(ts, violationId);

      invariant.commit(ts, *engine);
      invariant.recompute(ts + 1, *engine);
      ASSERT_EQ(notifiedViolation, engine->value(ts + 1, violationId));
    }
  }
}

class MockInDomain : public InSparseDomain {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    InSparseDomain::registerVars(engine);
  }

  MockInDomain(VarId violationId, VarId x, std::vector<DomainEntry> domain)
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
};
TEST_F(InSparseDomainTest, EngineIntegration) {
  std::vector<DomainEntry> dom{
      {-20, -15}, {-10, -5}, {0, 0}, {5, 10}, {15, 20}};

  const Int margin = 20;
  const Int lb = dom.front().lowerBound - margin;
  const Int ub = dom.back().upperBound + margin;

  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(lb, lb, ub);
    const VarId violationId = engine->makeIntVar(0, 0, 0);
    testNotifications<MockInDomain>(
        &engine->makeConstraint<MockInDomain>(violationId, x,
                                              std::vector<DomainEntry>(dom)),
        propMode, markingMode, 2, x, -5, violationId);
  }
}

}  // namespace
