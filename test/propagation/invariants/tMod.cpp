#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/mod.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ModTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarViewId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarViewId x, const VarViewId y) {
    Int denominator = _solver->value(ts, y);
    if (denominator == 0) {
      denominator = _solver->upperBound(y) > 0 ? 1 : -1;
    }
    return _solver->value(ts, x) % std::abs(denominator);
  }
};

TEST_F(ModTest, Examples) {
  std::vector<std::array<Int, 3>> data{
      {7, 4, 3}, {-7, 4, -3}, {7, -4, 3}, {-7, -4, -3}};

  Int xLb = std::numeric_limits<Int>::max();
  Int xUb = std::numeric_limits<Int>::min();
  Int yLb = std::numeric_limits<Int>::max();
  Int yUb = std::numeric_limits<Int>::min();
  Int outputLb = std::numeric_limits<Int>::max();
  Int outputUb = std::numeric_limits<Int>::min();

  for (const auto& [xVal, yVal, outputVal] : data) {
    xLb = std::min(xLb, xVal);
    xUb = std::max(xUb, xVal);
    yLb = std::min(yLb, yVal);
    yUb = std::max(yUb, yVal);
    outputLb = std::min(outputLb, outputVal);
    outputUb = std::max(outputUb, outputVal);
  }
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  EXPECT_TRUE(yLb != 0 || yUb != 0);

  _solver->open();
  const VarViewId x = _solver->makeIntVar(xUb, xLb, xUb);
  const VarViewId y = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId outputId = _solver->makeIntVar(0, outputLb, outputUb);
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, x, y);
  _solver->close();

  for (size_t i = 0; i < data.size(); ++i) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i + 1);
    const Int xVal = data.at(i).at(0);
    const Int yVal = data.at(i).at(1);
    const Int expectedOutput = data.at(i).at(2);

    _solver->setValue(ts, x, xVal);
    _solver->setValue(ts, y, yVal);

    invariant.recompute(ts);

    EXPECT_EQ(_solver->value(ts, outputId), expectedOutput);
    EXPECT_EQ(computeOutput(ts, x, y), expectedOutput);
  }
}

TEST_F(ModTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  _solver->open();
  const VarViewId x = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId y = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, x, y);
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

TEST_F(ModTest, Recompute) {
  const Int xLb = -1;
  const Int xUb = 0;
  const Int yLb = 0;
  const Int yUb = 1;
  const Int outputLb = -1;
  const Int outputUb = 0;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  EXPECT_TRUE(yLb != 0 || yUb != 0);

  _solver->open();
  const VarViewId x = _solver->makeIntVar(xUb, xLb, xUb);
  const VarViewId y = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId outputId = _solver->makeIntVar(outputLb, outputLb, outputUb);
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, x, y);
  _solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      _solver->setValue(_solver->currentTimestamp(), x, xVal);
      _solver->setValue(_solver->currentTimestamp(), y, yVal);

      const Int expectedOutput =
          computeOutput(_solver->currentTimestamp(), x, y);
      invariant.recompute(_solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                _solver->value(_solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  _solver->open();
  std::array<VarViewId, 2> inputs{_solver->makeIntVar(ub, lb, ub),
                                  _solver->makeIntVar(ub, lb, ub)};
  VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, inputs.at(0),
                                               inputs.at(1));
  _solver->close();

  Timestamp ts = _solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      _solver->setValue(_solver->currentTimestamp(), inputs.at(i), val);
      const Int expectedOutput =
          computeOutput(_solver->currentTimestamp(), inputs);

      invariant.notifyInputChanged(_solver->currentTimestamp(), LocalId(i));
      EXPECT_EQ(expectedOutput,
                _solver->value(_solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

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
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, inputs.at(0),
                                               inputs.at(1));
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

TEST_F(ModTest, NotifyCurrentInputChanged) {
  const Int lb = -10002;
  const Int ub = -10000;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarViewId, 2> inputs = {
      _solver->makeIntVar(valueDist(gen), lb, ub),
      _solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, inputs.at(0),
                                               inputs.at(1));
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

TEST_F(ModTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarViewId, 2> inputs{
      _solver->makeIntVar(committedValues.at(0), lb, ub),
      _solver->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, inputs.at(0),
                                               inputs.at(1));
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

TEST_F(ModTest, ZeroDenominator) {
  const Int xVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [yLb, yUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, 0}, {-50, 50, 0}, {0, 100, 0}}) {
    EXPECT_TRUE(yLb <= yUb);
    EXPECT_TRUE(yLb != 0 || yUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      _solver->open();
      const VarViewId x = _solver->makeIntVar(xVal, xVal, xVal);
      const VarViewId y = _solver->makeIntVar(0, yLb, yUb);
      const VarViewId outputId = _solver->makeIntVar(0, outputLb, outputUb);
      Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputId, x, y);
      _solver->close();

      EXPECT_EQ(expected, computeOutput(_solver->currentTimestamp(), x, y));
      if (method == 0) {
        invariant.recompute(_solver->currentTimestamp());
      } else {
        invariant.notifyInputChanged(_solver->currentTimestamp(), LocalId(1));
      }
      EXPECT_EQ(expected,
                _solver->value(_solver->currentTimestamp(), outputId));
    }
  }
}

class MockMod : public Mod {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Mod::registerVars();
  }
  explicit MockMod(SolverBase& solver, VarViewId output, VarViewId x,
                   VarViewId y)
      : Mod(solver, output, x, y) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Mod::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Mod::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Mod::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Mod::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Mod::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ModTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId x = _solver->makeIntVar(-10, -100, 100);
    const VarViewId y = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockMod>(
        &_solver->makeInvariant<MockMod>(*_solver, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace atlantis::testing
