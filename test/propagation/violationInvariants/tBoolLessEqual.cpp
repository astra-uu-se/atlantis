#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolLessEqualTest : public InvariantTest {
 public:
  bool isRegistered = false;

  Int computeViolation(const Timestamp ts,
                       const std::array<const VarViewId, 2>& inputs) {
    return computeViolation(_solver->value(ts, inputs.at(0)),
                            _solver->value(ts, inputs.at(1)));
  }

  static Int computeViolation(const std::array<const Int, 2>& inputs) {
    return computeViolation(inputs.at(0), inputs.at(1));
  }

  Int computeViolation(const Timestamp ts, const VarViewId x,
                       const VarViewId y) {
    return computeViolation(_solver->value(ts, x), _solver->value(ts, y));
  }

  static Int computeViolation(const Int xVal, const Int yVal) {
    if (xVal == 0 && yVal != 0) {
      return 1;
    }
    return 0;
  }
};

TEST_F(BoolLessEqualTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 10}, {1, 10}, {10, 100}};
  _solver->open();
  const VarViewId x = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId y = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  BoolLessEqual& invariant = _solver->makeViolationInvariant<BoolLessEqual>(
      *_solver, violationId, x, y);
  _solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    _solver->updateBounds(VarId(x), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      _solver->updateBounds(VarId(y), yLb, yUb, false);
      invariant.updateBounds(false);
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          _solver->setValue(_solver->currentTimestamp(), y, yVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
        }
      }
      ASSERT_GE(0, _solver->lowerBound(violationId));
      ASSERT_LE(1, _solver->upperBound(violationId));
    }
  }
}

TEST_F(BoolLessEqualTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 100;
  const Int yLb = 0;
  const Int yUb = 100;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  _solver->open();
  const std::array<const VarViewId, 2> inputs{
      _solver->makeIntVar(xUb, xLb, xUb), _solver->makeIntVar(yUb, yLb, yUb)};
  const VarViewId violationId =
      _solver->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  BoolLessEqual& invariant = _solver->makeViolationInvariant<BoolLessEqual>(
      *_solver, violationId, inputs.at(0), inputs.at(1));
  _solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      _solver->setValue(_solver->currentTimestamp(), inputs.at(0), xVal);
      _solver->setValue(_solver->currentTimestamp(), inputs.at(1), yVal);

      const Int expectedViolation = computeViolation(xVal, yVal);
      invariant.recompute(_solver->currentTimestamp());
      EXPECT_EQ(expectedViolation,
                _solver->value(_solver->currentTimestamp(), violationId));
    }
  }
}

TEST_F(BoolLessEqualTest, NotifyInputChanged) {
  const Int lb = 0;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const std::array<const VarViewId, 2> inputs{_solver->makeIntVar(ub, lb, ub),
                                              _solver->makeIntVar(ub, lb, ub)};
  const VarViewId violationId = _solver->makeIntVar(0, 0, ub - lb);
  BoolLessEqual& invariant = _solver->makeViolationInvariant<BoolLessEqual>(
      *_solver, violationId, inputs.at(0), inputs.at(1));
  _solver->close();

  Timestamp ts = _solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (LocalId i = 0; i < inputs.size(); ++i) {
      _solver->setValue(ts, inputs.at(i), val);
      const Int expectedViolation = computeViolation(ts, inputs);

      invariant.notifyInputChanged(ts, i);
      EXPECT_EQ(expectedViolation, _solver->value(ts, violationId));
    }
  }
}

TEST_F(BoolLessEqualTest, NextInput) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const std::array<const VarViewId, 2> inputs = {
      _solver->makeIntVar(0, lb, ub), _solver->makeIntVar(1, lb, ub)};
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
  BoolLessEqual& invariant = _solver->makeViolationInvariant<BoolLessEqual>(
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

TEST_F(BoolLessEqualTest, NotifyCurrentInputChanged) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<const VarViewId, 2> inputs = {
      _solver->makeIntVar(valueDist(gen), lb, ub),
      _solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarViewId violationId = _solver->makeIntVar(0, 0, ub - lb);
  BoolLessEqual& invariant = _solver->makeViolationInvariant<BoolLessEqual>(
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

TEST_F(BoolLessEqualTest, Commit) {
  const Int lb = 0;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::shuffle(indices.begin(), indices.end(), rng);

  _solver->open();
  const std::array<const VarViewId, 2> inputs{
      _solver->makeIntVar(committedValues.at(0), lb, ub),
      _solver->makeIntVar(committedValues.at(1), lb, ub)};

  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  BoolLessEqual& invariant = _solver->makeViolationInvariant<BoolLessEqual>(
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

class MockBoolLessEqual : public BoolLessEqual {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolLessEqual::registerVars();
  }
  explicit MockBoolLessEqual(SolverBase& solver, VarViewId violationId,
                             VarViewId x, VarViewId y)
      : BoolLessEqual(solver, violationId, x, y) {
    EXPECT_TRUE(violationId.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolLessEqual::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolLessEqual::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolLessEqual::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolLessEqual::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolLessEqual::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BoolLessEqualTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId x = _solver->makeIntVar(5, 0, 100);
    const VarViewId y = _solver->makeIntVar(0, 0, 100);
    const VarViewId viol = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockBoolLessEqual>(
        &_solver->makeViolationInvariant<MockBoolLessEqual>(*_solver, viol, x,
                                                            y),
        {propMode, markingMode, 3, x, 1, viol});
  }
}
}  // namespace atlantis::testing
