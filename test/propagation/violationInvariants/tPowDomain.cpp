#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/solver.hpp"
#include "propagation/violationInvariants/powDomain.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class PowDomainTest : public InvariantTest {
 public:
  Int computeViolation(Timestamp ts, const VarId x, const VarId y) {
    return computeViolation(solver->value(ts, x), solver->value(ts, y));
  }

  static Int computeViolation(const Int xVal, const Int yVal) {
    return xVal == 0 && yVal < 0 ? 1 : 0;
  }
};

TEST_F(PowDomainTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, -10}, {-5, 0}, {-2, 2}, {0, 5}, {10, 10}, {15, 20}};
  solver->open();
  const VarId x = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId violationId = solver->makeIntVar(0, 0, 1);
  PowDomain& invariant =
      solver->makeViolationInvariant<PowDomain>(*solver, violationId, x, y);
  solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    solver->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      solver->updateBounds(y, yLb, yUb, false);
      invariant.updateBounds(false);
      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        solver->setValue(solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          solver->setValue(solver->currentTimestamp(), y, yVal);
          invariant.updateBounds(false);
          invariant.recompute(solver->currentTimestamp());
          violations.emplace_back(
              solver->value(solver->currentTimestamp(), violationId));
        }
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_EQ(*minViol, solver->lowerBound(violationId));
      if (*maxViol != solver->upperBound(violationId)) {
        ASSERT_EQ(*maxViol, solver->upperBound(violationId));
      }
    }
  }
}

TEST_F(PowDomainTest, Recompute) {
  const Int xLb = -5;
  const Int xUb = 5;
  const Int yLb = -5;
  const Int yUb = 5;
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  solver->open();
  const VarId x = solver->makeIntVar(xUb, xLb, xUb);
  const VarId y = solver->makeIntVar(yUb, yLb, yUb);
  const VarId violationId = solver->makeIntVar(0, 0, 1);
  PowDomain& invariant =
      solver->makeViolationInvariant<PowDomain>(*solver, violationId, x, y);
  solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      solver->setValue(solver->currentTimestamp(), x, xVal);
      solver->setValue(solver->currentTimestamp(), y, yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedViolation,
                solver->value(solver->currentTimestamp(), violationId));
    }
  }
}

TEST_F(PowDomainTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::array<VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                              solver->makeIntVar(ub, lb, ub)};
  const VarId violationId = solver->makeIntVar(0, 0, 1);
  PowDomain& invariant = solver->makeViolationInvariant<PowDomain>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      solver->setValue(ts, inputs.at(i), val);
      const Int expectedViolation =
          computeViolation(ts, inputs.at(0), inputs.at(1));

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedViolation, solver->value(ts, violationId));
    }
  }
}

TEST_F(PowDomainTest, NextInput) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<VarId, 2> inputs = {solver->makeIntVar(lb, lb, ub),
                                       solver->makeIntVar(ub, lb, ub)};
  const VarId violationId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  PowDomain& invariant = solver->makeViolationInvariant<PowDomain>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
      const VarId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(PowDomainTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId violationId = solver->makeIntVar(0, 0, ub - lb);
  PowDomain& invariant = solver->makeViolationInvariant<PowDomain>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (const VarId& varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = solver->value(ts, varId);
      do {
        solver->setValue(ts, varId, valueDist(gen));
      } while (solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, violationId),
                computeViolation(ts, inputs.at(0), inputs.at(1)));
    }
  }
}

TEST_F(PowDomainTest, Commit) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      solver->makeIntVar(committedValues.at(0), lb, ub),
      solver->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarId violationId = solver->makeIntVar(0, 0, 2);
  PowDomain& invariant = solver->makeViolationInvariant<PowDomain>(
      *solver, violationId, inputs.at(0), inputs.at(1));
  solver->close();

  EXPECT_EQ(
      solver->value(solver->currentTimestamp(), violationId),
      computeViolation(solver->currentTimestamp(), inputs.at(0), inputs.at(1)));

  for (const size_t i : indices) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputs.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      solver->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == solver->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedViolation = solver->value(ts, violationId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, solver->value(ts, violationId));

    solver->commitIf(ts, inputs.at(i));
    committedValues.at(i) = solver->value(ts, inputs.at(i));
    solver->commitIf(ts, violationId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, solver->value(ts + 1, violationId));
  }
}

class MockPowDomain : public PowDomain {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    PowDomain::registerVars();
  }
  explicit MockPowDomain(SolverBase& solver, VarId violationId, VarId x,
                         VarId y)
      : PowDomain(solver, violationId, x, y) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return PowDomain::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return PowDomain::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          PowDomain::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          PowDomain::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      PowDomain::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(PowDomainTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId x = solver->makeIntVar(5, -100, 100);
    const VarId y = solver->makeIntVar(0, -100, 100);
    const VarId viol = solver->makeIntVar(0, 0, 1);
    testNotifications<MockPowDomain>(
        &solver->makeViolationInvariant<MockPowDomain>(*solver, viol, x, y),
        {propMode, markingMode, 3, x, 0, viol});
  }
}

}  // namespace atlantis::testing
