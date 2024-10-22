#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/times.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class TimesTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarViewId, 2> inputs) {
    return computeOutput(_solver->value(ts, inputs.at(0)),
                         _solver->value(ts, inputs.at(1)));
  }

  static Int computeOutput(std::array<Int, 2> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarViewId x, const VarViewId y) {
    return computeOutput(_solver->value(ts, x), _solver->value(ts, y));
  }

  static Int computeOutput(const Int xVal, const Int yVal) {
    return xVal * yVal;
  }
};

TEST_F(TimesTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  _solver->open();
  const VarViewId x = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId y = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  Times& invariant = _solver->makeInvariant<Times>(*_solver, outputId, x, y);
  _solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    _solver->updateBounds(VarId(x), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      _solver->updateBounds(VarId(y), yLb, yUb, false);
      _solver->open();
      invariant.updateBounds(false);
      _solver->close();
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          _solver->setValue(_solver->currentTimestamp(), y, yVal);
          invariant.recompute(_solver->currentTimestamp());
          const Int o = _solver->value(_solver->currentTimestamp(), outputId);
          if (o < _solver->lowerBound(outputId) ||
              _solver->upperBound(outputId) < o) {
            ASSERT_GE(o, _solver->lowerBound(outputId));
            ASSERT_LE(o, _solver->upperBound(outputId));
          }
        }
      }
    }
  }
}

TEST_F(TimesTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 0;
  const Int yUb = 5;
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  _solver->open();
  const VarViewId x = _solver->makeIntVar(xUb, xLb, xUb);
  const VarViewId y = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId outputId =
      _solver->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  Times& invariant = _solver->makeInvariant<Times>(*_solver, outputId, x, y);
  _solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      _solver->setValue(_solver->currentTimestamp(), x, xVal);
      _solver->setValue(_solver->currentTimestamp(), y, yVal);

      const Int expectedOutput = computeOutput(xVal, yVal);
      invariant.recompute(_solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                _solver->value(_solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(TimesTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::array<VarViewId, 2> inputs{_solver->makeIntVar(ub, lb, ub),
                                  _solver->makeIntVar(ub, lb, ub)};
  VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  Times& invariant = _solver->makeInvariant<Times>(*_solver, outputId,
                                                   inputs.at(0), inputs.at(1));
  _solver->close();

  Timestamp ts = _solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      _solver->setValue(ts, inputs.at(i), val);
      const Int expectedOutput = computeOutput(ts, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
    }
  }
}

TEST_F(TimesTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const std::array<VarViewId, 2> inputs = {_solver->makeIntVar(lb, lb, ub),
                                           _solver->makeIntVar(ub, lb, ub)};
  const VarViewId outputId = _solver->makeIntVar(0, 0, 2);
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
  Times& invariant = _solver->makeInvariant<Times>(*_solver, outputId,
                                                   inputs.at(0), inputs.at(1));
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

TEST_F(TimesTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarViewId, 2> inputs = {
      _solver->makeIntVar(valueDist(gen), lb, ub),
      _solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  Times& invariant = _solver->makeInvariant<Times>(*_solver, outputId,
                                                   inputs.at(0), inputs.at(1));
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
      EXPECT_EQ(_solver->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(TimesTest, Commit) {
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

  VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  Times& invariant = _solver->makeInvariant<Times>(*_solver, outputId,
                                                   inputs.at(0), inputs.at(1));
  _solver->close();

  EXPECT_EQ(_solver->value(_solver->currentTimestamp(), outputId),
            computeOutput(_solver->currentTimestamp(), inputs));

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
    const Int notifiedOutput = _solver->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

    _solver->commitIf(ts, VarId(inputs.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputs.at(i)));
    _solver->commitIf(ts, VarId(outputId));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputId));
  }
}

class MockTimes : public Times {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Times::registerVars();
  }
  explicit MockTimes(SolverBase& solver, VarViewId output, VarViewId x,
                     VarViewId y)
      : Times(solver, output, x, y) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Times::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Times::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Times::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Times::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Times::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(TimesTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId x = _solver->makeIntVar(-10, -100, 100);
    const VarViewId y = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockTimes>(
        &_solver->makeInvariant<MockTimes>(*_solver, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace atlantis::testing
