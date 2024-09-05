#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/pow.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class PowTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, const std::array<VarViewId, 2>& inputs) {
    return computeOutput(ts, inputs, 1);
  }
  Int computeOutput(Timestamp ts, const std::array<VarViewId, 2>& inputs,
                    Int zeroReplacement) {
    return computeOutput(_solver->value(ts, inputs.at(0)),
                         _solver->value(ts, inputs.at(1)), zeroReplacement);
  }

  static Int computeOutput(const std::array<Int, 2>& inputs) {
    return computeOutput(inputs.at(0), inputs.at(1), 1);
  }

  static Int computeOutput(const std::array<Int, 2>& inputs,
                           Int zeroReplacement) {
    return computeOutput(inputs.at(0), inputs.at(1), zeroReplacement);
  }

  Int computeOutput(Timestamp ts, const VarViewId base,
                    const VarViewId exponent) {
    return computeOutput(ts, base, exponent, 1);
  }

  Int computeOutput(Timestamp ts, const VarViewId base,
                    const VarViewId exponent, Int zeroReplacement) {
    return computeOutput(_solver->value(ts, base), _solver->value(ts, exponent),
                         zeroReplacement);
  }

  static Int computeOutput(const Int baseVal, const Int expVal) {
    return computeOutput(baseVal, expVal, 1);
  }

  static Int computeOutput(const Int baseVal, const Int expVal,
                           Int zeroReplacement) {
    if (baseVal == 0 && expVal < 0) {
      return static_cast<Int>(std::pow(zeroReplacement, expVal));
    }
    return static_cast<Int>(std::pow(baseVal, expVal));
  }
};

TEST_F(PowTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-8, -5}, {-3, 0}, {-2, 2}, {0, 3}, {5, 8}};
  _solver->open();
  const VarViewId base = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId exponent = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  Pow& invariant =
      _solver->makeInvariant<Pow>(*_solver, outputId, base, exponent);
  _solver->close();

  for (const auto& [baseLb, baseUb] : boundVec) {
    EXPECT_TRUE(baseLb <= baseUb);
    _solver->updateBounds(VarId(base), baseLb, baseUb, false);
    for (const auto& [expLb, expUb] : boundVec) {
      EXPECT_TRUE(expLb <= expUb);
      _solver->updateBounds(VarId(exponent), expLb, expUb, false);
      _solver->open();
      _solver->close();
      for (Int baseVal = baseLb; baseVal <= baseUb; ++baseVal) {
        _solver->setValue(_solver->currentTimestamp(), base, baseVal);
        for (Int expVal = expLb; expVal <= expUb; ++expVal) {
          _solver->setValue(_solver->currentTimestamp(), exponent, expVal);
          invariant.recompute(_solver->currentTimestamp());
          const Int o = _solver->value(_solver->currentTimestamp(), outputId);
          if (o < _solver->lowerBound(outputId) ||
              _solver->upperBound(outputId) < o) {
            invariant.updateBounds(false);
            ASSERT_GE(o, _solver->lowerBound(outputId));
            ASSERT_LE(o, _solver->upperBound(outputId));
          }
        }
      }
    }
  }
}

TEST_F(PowTest, Recompute) {
  const Int baseLb = 0;
  const Int baseUb = 10;
  const Int expLb = 0;
  const Int expUb = 5;
  EXPECT_TRUE(baseLb <= baseUb);
  EXPECT_TRUE(expLb <= expUb);

  _solver->open();
  const VarViewId base = _solver->makeIntVar(baseUb, baseLb, baseUb);
  const VarViewId exponent = _solver->makeIntVar(expUb, expLb, expUb);
  const VarViewId outputId =
      _solver->makeIntVar(0, 0, std::max(baseUb - expLb, expUb - baseLb));
  Pow& invariant =
      _solver->makeInvariant<Pow>(*_solver, outputId, base, exponent);
  _solver->close();

  for (Int baseVal = baseLb; baseVal <= baseUb; ++baseVal) {
    for (Int expVal = expLb; expVal <= expUb; ++expVal) {
      _solver->setValue(_solver->currentTimestamp(), base, baseVal);
      _solver->setValue(_solver->currentTimestamp(), exponent, expVal);

      const Int expectedOutput = computeOutput(baseVal, expVal);
      invariant.recompute(_solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                _solver->value(_solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(PowTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::array<VarViewId, 2> inputs{_solver->makeIntVar(ub, lb, ub),
                                  _solver->makeIntVar(ub, lb, ub)};
  VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  Pow& invariant = _solver->makeInvariant<Pow>(*_solver, outputId, inputs.at(0),
                                               inputs.at(1));
  _solver->close();

  Timestamp ts = _solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      _solver->setValue(ts, inputs.at(i), val);
      const Int expectedOutput = computeOutput(ts, inputs, 1);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
    }
  }
}

TEST_F(PowTest, NextInput) {
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
  Pow& invariant = _solver->makeInvariant<Pow>(*_solver, outputId, inputs.at(0),
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

TEST_F(PowTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarViewId, 2> inputs = {
      _solver->makeIntVar(valueDist(gen), lb, ub),
      _solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  Pow& invariant = _solver->makeInvariant<Pow>(*_solver, outputId, inputs.at(0),
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

TEST_F(PowTest, Commit) {
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
  Pow& invariant = _solver->makeInvariant<Pow>(*_solver, outputId, inputs.at(0),
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

class MockPow : public Pow {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Pow::registerVars();
  }
  explicit MockPow(SolverBase& solver, VarViewId output, VarViewId base,
                   VarViewId exponent)
      : Pow(solver, output, base, exponent) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Pow::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Pow::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Pow::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Pow::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Pow::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(PowTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId base = _solver->makeIntVar(-10, -100, 100);
    const VarViewId exponent = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockPow>(
        &_solver->makeInvariant<MockPow>(*_solver, output, base, exponent),
        {propMode, markingMode, 3, base, 0, output});
  }
}

}  // namespace atlantis::testing
