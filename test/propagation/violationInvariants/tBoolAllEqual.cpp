#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/violationInvariants/boolAllEqual.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolAllEqualTest : public InvariantTest {
 public:
  Int computeViolation(Timestamp ts, const std::vector<VarId>& vars) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = solver->value(ts, vars.at(i));
    }
    return computeViolation(values);
  }

  static Int computeViolation(const std::vector<Int>& values) {
    size_t numFalse = 0;
    size_t numTrue = 0;
    for (const Int val : values) {
      if (val == 0) {
        ++numTrue;
      } else {
        ++numFalse;
      }
    }
    return static_cast<Int>(std::min(values.size() - numTrue, values.size() - numFalse));
  }
};

TEST_F(BoolAllEqualTest, UpdateBounds) {}

TEST_F(BoolAllEqualTest, Recompute) {
  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 5}, {10, 10}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_TRUE(lb <= ub);
    solver->open();
    const VarId a = solver->makeIntVar(lb, lb, ub);
    const VarId b = solver->makeIntVar(lb, lb, ub);
    const VarId c = solver->makeIntVar(lb, lb, ub);
    const VarId violationId = solver->makeIntVar(0, 0, 2);
    BoolAllEqual& invariant = solver->makeViolationInvariant<BoolAllEqual>(
        *solver, violationId, std::vector<VarId>{a, b, c});
    solver->close();

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          solver->setValue(solver->currentTimestamp(), a, aVal);
          solver->setValue(solver->currentTimestamp(), b, bVal);
          solver->setValue(solver->currentTimestamp(), c, cVal);
          const Int expectedViolation =
              computeViolation(std::vector{aVal, bVal, cVal});
          invariant.recompute(solver->currentTimestamp());
          EXPECT_EQ(expectedViolation,
                    solver->value(solver->currentTimestamp(), violationId));
        }
      }
    }
  }
}

TEST_F(BoolAllEqualTest, NotifyInputChanged) {
  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 5}, {10, 10}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_TRUE(lb <= ub);

    solver->open();
    std::vector<VarId> inputs{solver->makeIntVar(lb, lb, ub),
                              solver->makeIntVar(lb, lb, ub),
                              solver->makeIntVar(lb, lb, ub)};
    const VarId violationId = solver->makeIntVar(0, 0, 2);
    BoolAllEqual& invariant = solver->makeViolationInvariant<BoolAllEqual>(
        *solver, violationId, std::vector<VarId>(inputs));
    solver->close();

    Timestamp ts = solver->currentTimestamp();

    for (Int val = lb; val <= ub; ++val) {
      ++ts;
      for (size_t i = 0; i < inputs.size(); ++i) {
        solver->setValue(ts, inputs[i], val);
        const Int expectedViolation = computeViolation(ts, inputs);

        invariant.notifyInputChanged(ts, LocalId(i));
        EXPECT_EQ(expectedViolation, solver->value(ts, violationId));
      }
    }
  }
}

TEST_F(BoolAllEqualTest, NextInput) {
  const Int numInputs = 1000;
  const Int lb = 0;
  const Int ub = numInputs - 1;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarId> inputs;
  for (Int i = 0; i < numInputs; ++i) {
    inputs.emplace_back(solver->makeIntVar(i, lb, ub));
  }
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId violationId = solver->makeIntVar(0, 0, 2);
  BoolAllEqual& invariant = solver->makeViolationInvariant<BoolAllEqual>(
      *solver, violationId, std::vector<VarId>(inputs));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
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

TEST_F(BoolAllEqualTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const size_t numInputs = 100;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(solver->makeIntVar(valueDist(gen), lb, ub));
  }
  const VarId violationId = solver->makeIntVar(0, 0, numInputs - 1);
  BoolAllEqual& invariant = solver->makeViolationInvariant<BoolAllEqual>(
      *solver, violationId, std::vector<VarId>(inputs));
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
      EXPECT_EQ(solver->value(ts, violationId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(BoolAllEqualTest, Commit) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const size_t numInputs = 1000;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<size_t> varDist(size_t(0), numInputs);
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(valueDist(gen));
    inputs.emplace_back(solver->makeIntVar(committedValues.back(), lb, ub));
  }
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarId violationId = solver->makeIntVar(0, 0, 2);
  BoolAllEqual& invariant = solver->makeViolationInvariant<BoolAllEqual>(
      *solver, violationId, std::vector<VarId>(inputs));
  solver->close();

  EXPECT_EQ(solver->value(solver->currentTimestamp(), violationId),
            computeViolation(solver->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
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

class MockAllDifferent : public BoolAllEqual {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolAllEqual::registerVars();
  }
  explicit MockAllDifferent(SolverBase& solver, VarId violationId,
                            std::vector<VarId>&& t_vars)
      : BoolAllEqual(solver, violationId, std::move(t_vars)) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolAllEqual::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolAllEqual::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolAllEqual::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolAllEqual::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolAllEqual::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(BoolAllEqualTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    std::vector<VarId> args;
    const size_t numArgs = 10;
    for (size_t value = 0; value < numArgs; ++value) {
      args.emplace_back(solver->makeIntVar(0, -100, 100));
    }
    const VarId viol = solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    const VarId modifiedVarId = args.front();
    testNotifications<MockAllDifferent>(
        &solver->makeViolationInvariant<MockAllDifferent>(*solver, viol, std::move(args)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 1, viol});
  }
}

}  // namespace atlantis::testing
