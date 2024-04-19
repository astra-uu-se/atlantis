#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/allDifferentExcept.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class AllDifferentExceptTest : public InvariantTest {
 public:
  Int computeViolation(const Timestamp ts, const std::vector<VarViewId>& vars,
                       const std::unordered_set<Int>& ignoredSet) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = _solver->value(ts, vars.at(i));
    }
    return computeViolation(values, ignoredSet);
  }

  static Int computeViolation(const std::vector<Int>& values,
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
  _solver->open();
  std::vector<VarViewId> inputs{_solver->makeIntVar(0, 0, 0),
                                _solver->makeIntVar(0, 0, 0),
                                _solver->makeIntVar(0, 0, 0)};
  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  AllDifferentExcept& invariant =
      _solver->makeViolationInvariant<AllDifferentExcept>(
          *_solver, violationId, std::vector<VarViewId>(inputs),
          std::vector<Int>{10, 20});

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_TRUE(aLb <= aUb);
    _solver->updateBounds(VarId(inputs.at(0)), aLb, aUb, false);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_TRUE(bLb <= bUb);
      _solver->updateBounds(VarId(inputs.at(2)), bLb, bUb, false);
      for (const auto& [cLb, cUb] : boundVec) {
        EXPECT_TRUE(cLb <= cUb);
        _solver->updateBounds(VarId(inputs.at(2)), cLb, cUb, false);
        invariant.updateBounds(false);
        ASSERT_EQ(0, _solver->lowerBound(violationId));
        ASSERT_EQ(inputs.size() - 1, _solver->upperBound(violationId));
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

    _solver->open();
    const VarViewId a = _solver->makeIntVar(lb, lb, ub);
    const VarViewId b = _solver->makeIntVar(lb, lb, ub);
    const VarViewId c = _solver->makeIntVar(lb, lb, ub);
    const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
    AllDifferentExcept& invariant =
        _solver->makeViolationInvariant<AllDifferentExcept>(
            *_solver, violationId, std::vector<VarViewId>{a, b, c},
            std::vector<Int>(ignored));
    _solver->close();

    const std::vector<VarViewId> inputs{a, b, c};

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          _solver->setValue(_solver->currentTimestamp(), a, aVal);
          _solver->setValue(_solver->currentTimestamp(), b, bVal);
          _solver->setValue(_solver->currentTimestamp(), c, cVal);
          const Int expectedViolation =
              computeViolation(_solver->currentTimestamp(), inputs, ignoredSet);
          invariant.recompute(_solver->currentTimestamp());
          EXPECT_EQ(expectedViolation,
                    _solver->value(_solver->currentTimestamp(), violationId));
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

    _solver->open();
    std::vector<VarViewId> inputs{_solver->makeIntVar(lb, lb, ub),
                                  _solver->makeIntVar(lb, lb, ub),
                                  _solver->makeIntVar(lb, lb, ub)};
    const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
    AllDifferentExcept& invariant =
        _solver->makeViolationInvariant<AllDifferentExcept>(
            *_solver, violationId, std::vector<VarViewId>(inputs),
            std::vector<Int>(ignored));
    _solver->close();

    Timestamp ts = _solver->currentTimestamp();

    for (Int val = lb; val <= ub; ++val) {
      ++ts;
      for (size_t j = 0; j < inputs.size(); ++j) {
        _solver->setValue(ts, inputs[j], val);
        const Int expectedViolation = computeViolation(ts, inputs, ignoredSet);

        invariant.notifyInputChanged(ts, LocalId(j));
        EXPECT_EQ(expectedViolation, _solver->value(ts, violationId));
      }
    }
  }
}

TEST_F(AllDifferentExceptTest, NextInput) {
  const size_t numInputs = 1000;
  const Int lb = 0;
  const Int ub = numInputs - 1;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarViewId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(_solver->makeIntVar(static_cast<Int>(i), lb, ub));
  }

  std::vector<Int> ignored;
  for (size_t i = 0; i < numInputs; i += 3) {
    ignored.emplace_back(i);
  }

  const VarViewId minVarId =
      *std::min_element(inputs.begin(), inputs.end(),
                        [&](const VarViewId& a, const VarViewId& b) {
                          return size_t(a) < size_t(b);
                        });
  const VarViewId maxVarId =
      *std::max_element(inputs.begin(), inputs.end(),
                        [&](const VarViewId& a, const VarViewId& b) {
                          return size_t(a) < size_t(b);
                        });

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  AllDifferentExcept& invariant =
      _solver->makeViolationInvariant<AllDifferentExcept>(
          *_solver, violationId, std::vector<VarViewId>(inputs), ignored);
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(size_t(maxVarId) + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
      const VarViewId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_LE(size_t(minVarId), size_t(varId));
      EXPECT_GE(size_t(maxVarId), size_t(varId));
      EXPECT_FALSE(notified.at(size_t(varId)));
      notified.at(size_t(varId)) = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t i = size_t(minVarId); i <= size_t(maxVarId); ++i) {
      EXPECT_TRUE(notified.at(i));
    }
  }
}

TEST_F(AllDifferentExceptTest, NotifyCurrentInputChanged) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const size_t numInputs = 100;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::vector<VarViewId> inputs;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(_solver->makeIntVar(valueDist(gen), lb, ub));
  }
  std::vector<Int> ignored;
  for (size_t i = 0; i < numInputs; i += 3) {
    ignored.emplace_back(i);
  }
  std::unordered_set<Int> ignoredSet;
  std::copy(ignored.begin(), ignored.end(),
            std::inserter(ignoredSet, ignoredSet.end()));

  const VarViewId violationId = _solver->makeIntVar(0, 0, numInputs - 1);
  AllDifferentExcept& invariant =
      _solver->makeViolationInvariant<AllDifferentExcept>(
          *_solver, violationId, std::vector<VarViewId>(inputs), ignored);
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, valueDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, violationId),
                computeViolation(ts, inputs, ignoredSet));
    }
  }
}

TEST_F(AllDifferentExceptTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const size_t numInputs = 1000;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<size_t> varDist(size_t(0), numInputs);
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarViewId> inputs;
  std::vector<Int> ignored;
  for (size_t i = 0; i < numInputs; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(valueDist(gen));
    inputs.emplace_back(_solver->makeIntVar(committedValues.back(), lb, ub));
    ignored.emplace_back(i);
  }
  std::unordered_set<Int> ignoredSet;
  std::copy(ignored.begin(), ignored.end(),
            std::inserter(ignoredSet, ignoredSet.end()));
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  AllDifferentExcept& invariant =
      _solver->makeViolationInvariant<AllDifferentExcept>(
          *_solver, violationId, std::vector<VarViewId>(inputs), ignored);
  _solver->close();

  EXPECT_EQ(_solver->value(_solver->currentTimestamp(), violationId),
            computeViolation(_solver->currentTimestamp(), inputs, ignoredSet));

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == _solver->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId{i});

    // incremental value
    const Int notifiedViolation = _solver->value(ts, violationId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, _solver->value(ts, violationId));

    _solver->commitIf(ts, VarId(inputs.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputs.at(i)));
    _solver->commitIf(ts, VarId(violationId));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, _solver->value(ts + 1, violationId));
  }
}

class MockAllDifferentExcept : public AllDifferentExcept {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    AllDifferentExcept::registerVars();
  }
  explicit MockAllDifferentExcept(SolverBase& solver, VarViewId violationId,
                                  std::vector<VarViewId>&& vars,
                                  const std::vector<Int>& ignored)
      : AllDifferentExcept(solver, violationId, std::move(vars), ignored) {
    EXPECT_TRUE(violationId.isVar());

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
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(AllDifferentExceptTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<VarViewId> args;
    const size_t numArgs = 10;
    const Int lb = -100;
    const Int ub = 100;
    for (size_t value = 0; value < numArgs; ++value) {
      args.emplace_back(_solver->makeIntVar(0, lb, ub));
    }
    std::vector<Int> ignored(ub - lb, 0);
    for (size_t i = 0; i < ignored.size(); ++i) {
      ignored[i] = static_cast<Int>(i) - lb;
    }
    std::shuffle(ignored.begin(), ignored.end(), rng);
    const VarViewId viol = _solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    const VarViewId modifiedVarId = args.front();
    testNotifications<MockAllDifferentExcept>(
        &_solver->makeViolationInvariant<MockAllDifferentExcept>(
            *_solver, viol, std::move(args), ignored),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 1, viol});
  }
}

}  // namespace atlantis::testing
