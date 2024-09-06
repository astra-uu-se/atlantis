#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class LessEqualTest : public InvariantTest {
 public:
  Int computeViolation(Timestamp ts, std::array<VarViewId, 2> inputs) {
    return computeViolation(_solver->value(ts, inputs.at(0)),
                            _solver->value(ts, inputs.at(1)));
  }

  static Int computeViolation(std::array<Int, 2> inputs) {
    return computeViolation(inputs.at(0), inputs.at(1));
  }

  Int computeViolation(Timestamp ts, const VarViewId x, const VarViewId y) {
    return computeViolation(_solver->value(ts, x), _solver->value(ts, y));
  }

  static Int computeViolation(const Int xVal, const Int yVal) {
    if (xVal <= yVal) {
      return 0;
    }
    return xVal - yVal;
  }
};

TEST_F(LessEqualTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  _solver->open();
  const VarViewId x = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId y = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  LessEqual& invariant =
      _solver->makeViolationInvariant<LessEqual>(*_solver, violationId, x, y);
  _solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    _solver->updateBounds(VarId(x), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      _solver->updateBounds(VarId(y), yLb, yUb, false);
      invariant.updateBounds(false);
      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          _solver->setValue(_solver->currentTimestamp(), y, yVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
          violations.emplace_back(
              _solver->value(_solver->currentTimestamp(), violationId));
        }
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_EQ(*minViol, _solver->lowerBound(violationId));
      ASSERT_EQ(*maxViol, _solver->upperBound(violationId));
    }
  }
}

TEST_F(LessEqualTest, Recompute) {
  const Int xLb = -100;
  const Int xUb = 25;
  const Int yLb = -25;
  const Int yUb = 50;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  _solver->open();
  const VarViewId x = _solver->makeIntVar(xUb, xLb, xUb);
  const VarViewId y = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId violationId =
      _solver->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  LessEqual& invariant =
      _solver->makeViolationInvariant<LessEqual>(*_solver, violationId, x, y);
  _solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      _solver->setValue(_solver->currentTimestamp(), x, xVal);
      _solver->setValue(_solver->currentTimestamp(), y, yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(_solver->currentTimestamp());
      EXPECT_EQ(expectedViolation,
                _solver->value(_solver->currentTimestamp(), violationId));
    }
  }
}

TEST_F(LessEqualTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::array<VarViewId, 2> inputs{_solver->makeIntVar(ub, lb, ub),
                                  _solver->makeIntVar(ub, lb, ub)};
  const VarViewId violationId = _solver->makeIntVar(0, 0, ub - lb);
  LessEqual& invariant = _solver->makeViolationInvariant<LessEqual>(
      *_solver, violationId, inputs.at(0), inputs.at(1));
  _solver->close();

  for (Int val = lb; val <= ub; ++val) {
    for (size_t i = 0; i < inputs.size(); ++i) {
      _solver->setValue(_solver->currentTimestamp(), inputs.at(i), val);
      const Int expectedViolation =
          computeViolation(_solver->currentTimestamp(), inputs);

      invariant.notifyInputChanged(_solver->currentTimestamp(), LocalId(i));
      EXPECT_EQ(expectedViolation,
                _solver->value(_solver->currentTimestamp(), violationId));
    }
  }
}

TEST_F(LessEqualTest, NextInput) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const std::array<VarViewId, 2> inputs = {_solver->makeIntVar(0, lb, ub),
                                           _solver->makeIntVar(1, lb, ub)};
  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
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

  LessEqual& invariant = _solver->makeViolationInvariant<LessEqual>(
      *_solver, violationId, inputs.at(0), inputs.at(1));
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(size_t(maxVarId) + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
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

TEST_F(LessEqualTest, NotifyCurrentInputChanged) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarViewId, 2> inputs = {
      _solver->makeIntVar(valueDist(gen), lb, ub),
      _solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarViewId violationId = _solver->makeIntVar(0, 0, ub - lb);
  LessEqual& invariant = _solver->makeViolationInvariant<LessEqual>(
      *_solver, violationId, inputs.at(0), inputs.at(1));
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
      EXPECT_EQ(_solver->value(ts, violationId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(LessEqualTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarViewId, 2> inputs{
      _solver->makeIntVar(committedValues.at(0), lb, ub),
      _solver->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  LessEqual& invariant = _solver->makeViolationInvariant<LessEqual>(
      *_solver, violationId, inputs.at(0), inputs.at(1));
  _solver->close();

  EXPECT_EQ(_solver->value(_solver->currentTimestamp(), violationId),
            computeViolation(_solver->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputs.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == _solver->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

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

class MockLessEqual : public LessEqual {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    LessEqual::registerVars();
  }
  explicit MockLessEqual(SolverBase& solver, VarViewId violationId, VarViewId x,
                         VarViewId y)
      : LessEqual(solver, violationId, x, y) {
    EXPECT_TRUE(violationId.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return LessEqual::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return LessEqual::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          LessEqual::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          LessEqual::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      LessEqual::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(LessEqualTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId x = _solver->makeIntVar(5, -100, 100);
    const VarViewId y = _solver->makeIntVar(0, -100, 100);
    const VarViewId viol = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockLessEqual>(
        &_solver->makeViolationInvariant<MockLessEqual>(*_solver, viol, x, y),
        {propMode, markingMode, 3, x, -5, viol});
  }
}

}  // namespace atlantis::testing
