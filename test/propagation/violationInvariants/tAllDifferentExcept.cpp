#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/violationInvariants/allDifferentExcept.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class AllDifferentExceptTest : public InvariantTest {
 public:
  Int computeViolation(const Timestamp ts, const std::vector<VarId>& vars,
                       const std::unordered_set<Int>& ignoredSet) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = solver->value(ts, vars.at(i));
    }
    return computeViolation(values, ignoredSet);
  }

  Int computeViolation(const std::vector<Int>& values,
                       const std::unordered_set<Int>& ignoredSet) {
    std::vector<bool> checked(values.size(), false);
    Int expectedViolation = 0;
    for (size_t i = 0; i < values.size(); ++i) {
      if (checked[i] || ignoredSet.count(values[i]) > 0) {
        continue;
      }
      checked[i] = true;
      for (size_t j = i + 1; j < values.size(); ++j) {
        if (checked[j]) {
          continue;
        }
        if (values[i] == values[j]) {
          checked[j] = true;
          ++expectedViolation;
        }
      }
    }
    return expectedViolation;
  }
};

TEST_F(AllDifferentExceptTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-250, -150}, {-100, 0}, {-50, 50}, {0, 100}, {150, 250}};
  solver->open();
  std::vector<VarId> inputs{solver->makeIntVar(0, 0, 0),
                            solver->makeIntVar(0, 0, 0),
                            solver->makeIntVar(0, 0, 0)};
  const VarId violationId = solver->makeIntVar(0, 0, 2);
  AllDifferentExcept& invariant = solver->makeViolationInvariant<AllDifferentExcept>(
      *solver, violationId, std::vector<VarId>(inputs),
      std::vector<Int>{10, 20});

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_TRUE(aLb <= aUb);
    solver->updateBounds(inputs.at(0), aLb, aUb, false);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_TRUE(bLb <= bUb);
      solver->updateBounds(inputs.at(2), bLb, bUb, false);
      for (const auto& [cLb, cUb] : boundVec) {
        EXPECT_TRUE(cLb <= cUb);
        solver->updateBounds(inputs.at(2), cLb, cUb, false);
        invariant.updateBounds();
        ASSERT_EQ(0, solver->lowerBound(violationId));
        ASSERT_EQ(inputs.size() - 1, solver->upperBound(violationId));
      }
    }
  }
}

TEST_F(AllDifferentExceptTest, Recompute) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-10004, -10000}, {-1, 1}, {10000, 10002}};

  std::vector<std::vector<Int>> ignoredVec{
      {-10003, -10000}, {-2, -1, 2}, {10000}};

  for (size_t i = 0; i < boundVec.size(); ++i) {
    auto const [lb, ub] = boundVec[i];
    auto const ignored = ignoredVec[i];
    std::unordered_set<Int> ignoredSet;
    std::copy(ignored.begin(), ignored.end(),
              std::inserter(ignoredSet, ignoredSet.end()));
    EXPECT_TRUE(lb <= ub);

    solver->open();
    const VarId a = solver->makeIntVar(lb, lb, ub);
    const VarId b = solver->makeIntVar(lb, lb, ub);
    const VarId c = solver->makeIntVar(lb, lb, ub);
    const VarId violationId = solver->makeIntVar(0, 0, 2);
    AllDifferentExcept& invariant = solver->makeViolationInvariant<AllDifferentExcept>(
        *solver, violationId, std::vector<VarId>{a, b, c},
        std::vector<Int>(ignored));
    solver->close();

    const std::vector<VarId> inputs{a, b, c};

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          solver->setValue(solver->currentTimestamp(), a, aVal);
          solver->setValue(solver->currentTimestamp(), b, bVal);
          solver->setValue(solver->currentTimestamp(), c, cVal);
          const Int expectedViolation =
              computeViolation(solver->currentTimestamp(), inputs, ignoredSet);
          invariant.recompute(solver->currentTimestamp());
          EXPECT_EQ(expectedViolation,
                    solver->value(solver->currentTimestamp(), violationId));
        }
      }
    }
  }
}

TEST_F(AllDifferentExceptTest, NotifyInputChanged) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  std::vector<std::vector<Int>> ignoredVec{
      {-10003, -10000}, {-2, -1, 2}, {10000}};

  for (size_t i = 0; i < boundVec.size(); ++i) {
    auto const [lb, ub] = boundVec[i];
    auto const ignored = ignoredVec[i];
    std::unordered_set<Int> ignoredSet;
    std::copy(ignored.begin(), ignored.end(),
              std::inserter(ignoredSet, ignoredSet.end()));
    EXPECT_TRUE(lb <= ub);

    solver->open();
    std::vector<VarId> inputs{solver->makeIntVar(lb, lb, ub),
                              solver->makeIntVar(lb, lb, ub),
                              solver->makeIntVar(lb, lb, ub)};
    const VarId violationId = solver->makeIntVar(0, 0, 2);
    AllDifferentExcept& invariant = solver->makeViolationInvariant<AllDifferentExcept>(
        *solver, violationId, std::vector<VarId>(inputs),
        std::vector<Int>(ignored));
    solver->close();

    Timestamp ts = solver->currentTimestamp();

    for (Int val = lb; val <= ub; ++val) {
      ++ts;
      for (size_t j = 0; j < inputs.size(); ++j) {
        solver->setValue(ts, inputs[j], val);
        const Int expectedViolation = computeViolation(ts, inputs, ignoredSet);

        invariant.notifyInputChanged(ts, LocalId(j));
        EXPECT_EQ(expectedViolation, solver->value(ts, violationId));
      }
    }
  }
}

TEST_F(AllDifferentExceptTest, NextInput) {
  const size_t numInputs = 1000;
  const Int lb = 0;
  const Int ub = numInputs - 1;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(solver->makeIntVar(i, lb, ub));
  }

  std::vector<Int> ignored;
  for (size_t i = 0; i < numInputs; i += 3) {
    ignored.emplace_back(i);
  }

  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId violationId = solver->makeIntVar(0, 0, 2);
  AllDifferentExcept& invariant = solver->makeViolationInvariant<AllDifferentExcept>(
      *solver, violationId, std::vector<VarId>(inputs), ignored);
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

TEST_F(AllDifferentExceptTest, NotifyCurrentInputChanged) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const size_t numInputs = 100;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::vector<VarId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(solver->makeIntVar(valueDist(gen), lb, ub));
  }
  std::vector<Int> ignored;
  for (size_t i = 0; i < numInputs; i += 3) {
    ignored.emplace_back(i);
  }
  std::unordered_set<Int> ignoredSet;
  std::copy(ignored.begin(), ignored.end(),
            std::inserter(ignoredSet, ignoredSet.end()));

  const VarId violationId = solver->makeIntVar(0, 0, numInputs - 1);
  AllDifferentExcept& invariant = solver->makeViolationInvariant<AllDifferentExcept>(
      *solver, violationId, std::vector<VarId>(inputs), ignored);
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = solver->value(ts, varId);
      do {
        solver->setValue(ts, varId, valueDist(gen));
      } while (solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, violationId),
                computeViolation(ts, inputs, ignoredSet));
    }
  }
}

TEST_F(AllDifferentExceptTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const size_t numInputs = 1000;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<size_t> varDist(size_t(0), numInputs);
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarId> inputs;
  std::vector<Int> ignored;
  for (size_t i = 0; i < numInputs; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(valueDist(gen));
    inputs.emplace_back(solver->makeIntVar(committedValues.back(), lb, ub));
    ignored.emplace_back(i);
  }
  std::unordered_set<Int> ignoredSet;
  std::copy(ignored.begin(), ignored.end(),
            std::inserter(ignoredSet, ignoredSet.end()));
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarId violationId = solver->makeIntVar(0, 0, 2);
  AllDifferentExcept& invariant = solver->makeViolationInvariant<AllDifferentExcept>(
      *solver, violationId, std::vector<VarId>(inputs), ignored);
  solver->close();

  EXPECT_EQ(solver->value(solver->currentTimestamp(), violationId),
            computeViolation(solver->currentTimestamp(), inputs, ignoredSet));

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

class MockAllDifferentExcept : public AllDifferentExcept {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    AllDifferentExcept::registerVars();
  }
  explicit MockAllDifferentExcept(SolverBase& solver, VarId violationId,
                                  std::vector<VarId> vars,
                                  const std::vector<Int>& ignored)
      : AllDifferentExcept(solver, violationId, vars, ignored) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return AllDifferentExcept::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return AllDifferentExcept::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          AllDifferentExcept::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          AllDifferentExcept::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      AllDifferentExcept::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(AllDifferentExceptTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    std::vector<VarId> args;
    const size_t numArgs = 10;
    const Int lb = -100;
    const Int ub = 100;
    for (size_t value = 0; value < numArgs; ++value) {
      args.emplace_back(solver->makeIntVar(0, lb, ub));
    }
    std::vector<Int> ignored(ub - lb, 0);
    for (size_t i = 0; i < ignored.size(); ++i) {
      ignored[i] = i - lb;
    }
    std::shuffle(ignored.begin(), ignored.end(), rng);
    const VarId viol = solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    testNotifications<MockAllDifferentExcept>(
        &solver->makeViolationInvariant<MockAllDifferentExcept>(*solver, viol, args,
                                                        ignored),
        {propMode, markingMode, numArgs + 1, args.front(), 1, viol});
  }
}

}  // namespace atlantis::testing