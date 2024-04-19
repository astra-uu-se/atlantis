#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IntDivTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarViewId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarViewId numerator,
                    const VarViewId denominator) {
    Int denVal = _solver->value(ts, denominator);
    if (denVal == 0) {
      denVal = _solver->upperBound(denominator) > 0 ? 1 : -1;
    }
    return _solver->value(ts, numerator) / denVal;
  }
};

TEST_F(IntDivTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  _solver->open();
  const VarViewId numerator = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId denominator = _solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  IntDiv& invariant = _solver->makeInvariant<IntDiv>(*_solver, outputId,
                                                     numerator, denominator);
  _solver->close();

  for (const auto& [nomLb, nomUb] : boundVec) {
    EXPECT_TRUE(nomLb <= nomUb);
    _solver->updateBounds(VarId(numerator), nomLb, nomUb, false);
    for (const auto& [denLb, denUb] : boundVec) {
      EXPECT_TRUE(denLb <= denUb);
      _solver->updateBounds(VarId(denominator), denLb, denUb, false);
      _solver->open();
      invariant.updateBounds(false);
      _solver->close();
      std::vector<Int> outputs;
      const Int outLb = _solver->lowerBound(outputId);
      const Int outUb = _solver->upperBound(outputId);
      for (Int nomVal = nomLb; nomVal <= nomUb; ++nomVal) {
        _solver->setValue(_solver->currentTimestamp(), numerator, nomVal);
        for (Int denVal = denLb; denVal <= denUb; ++denVal) {
          _solver->setValue(_solver->currentTimestamp(), denominator, denVal);
          invariant.recompute(_solver->currentTimestamp());
          const Int outVal =
              _solver->value(_solver->currentTimestamp(), outputId);
          if (outVal < outLb || outUb < outVal) {
            ASSERT_TRUE(outLb <= outVal);
            ASSERT_TRUE(outVal <= outUb);
          }
          outputs.emplace_back(outVal);
        }
      }
      const auto& [minVal, maxVal] =
          std::minmax_element(outputs.begin(), outputs.end());
      if (*minVal != _solver->lowerBound(outputId)) {
        ASSERT_EQ(*minVal, _solver->lowerBound(outputId));
      }
      ASSERT_EQ(*maxVal, _solver->upperBound(outputId));
    }
  }
}

TEST_F(IntDivTest, Recompute) {
  const Int nomLb = -1;
  const Int nomUb = 0;
  const Int denLb = 0;
  const Int denUb = 1;
  const Int outputLb = -1;
  const Int outputUb = 0;

  EXPECT_TRUE(nomLb <= nomUb);
  EXPECT_TRUE(denLb <= denUb);
  EXPECT_TRUE(denLb != 0 || denUb != 0);

  _solver->open();
  const VarViewId numerator = _solver->makeIntVar(nomUb, nomLb, nomUb);
  const VarViewId denominator = _solver->makeIntVar(denUb, denLb, denUb);
  const VarViewId outputId = _solver->makeIntVar(0, outputLb, outputUb);
  IntDiv& invariant = _solver->makeInvariant<IntDiv>(*_solver, outputId,
                                                     numerator, denominator);
  _solver->close();

  for (Int nomVal = nomLb; nomVal <= nomUb; ++nomVal) {
    for (Int denVal = denLb; denVal <= denUb; ++denVal) {
      _solver->setValue(_solver->currentTimestamp(), numerator, nomVal);
      _solver->setValue(_solver->currentTimestamp(), denominator, denVal);

      const Int expectedOutput =
          computeOutput(_solver->currentTimestamp(), numerator, denominator);
      invariant.recompute(_solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                _solver->value(_solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(IntDivTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  _solver->open();
  std::array<VarViewId, 2> inputs{_solver->makeIntVar(ub, lb, ub),
                                  _solver->makeIntVar(ub, lb, ub)};
  VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  IntDiv& invariant = _solver->makeInvariant<IntDiv>(
      *_solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, NextInput) {
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
  IntDiv& invariant = _solver->makeInvariant<IntDiv>(
      *_solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, NotifyCurrentInputChanged) {
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
  IntDiv& invariant = _solver->makeInvariant<IntDiv>(
      *_solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, Commit) {
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
  IntDiv& invariant = _solver->makeInvariant<IntDiv>(
      *_solver, outputId, inputs.at(0), inputs.at(1));
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
    invariant.notifyInputChanged(ts, LocalId{i});

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

TEST_F(IntDivTest, ZeroDenominator) {
  const Int nomVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [denLb, denUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, -10}, {-50, 50, 10}, {0, 100, 10}}) {
    EXPECT_TRUE(denLb <= denUb);
    EXPECT_TRUE(denLb != 0 || denUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      _solver->open();
      const VarViewId numerator = _solver->makeIntVar(nomVal, nomVal, nomVal);
      const VarViewId denominator = _solver->makeIntVar(0, denLb, denUb);
      const VarViewId outputId = _solver->makeIntVar(0, outputLb, outputUb);
      IntDiv& invariant = _solver->makeInvariant<IntDiv>(
          *_solver, outputId, numerator, denominator);
      _solver->close();

      EXPECT_EQ(expected, computeOutput(_solver->currentTimestamp(), numerator,
                                        denominator));
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

class MockIntDiv : public IntDiv {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    IntDiv::registerVars();
  }
  explicit MockIntDiv(SolverBase& solver, VarViewId output, VarViewId numerator,
                      VarViewId denominator)
      : IntDiv(solver, output, numerator, denominator) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return IntDiv::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return IntDiv::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          IntDiv::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          IntDiv::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      IntDiv::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(IntDivTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId numerator = _solver->makeIntVar(-10, -100, 100);
    const VarViewId denominator = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockIntDiv>(
        &_solver->makeInvariant<MockIntDiv>(*_solver, output, numerator,
                                            denominator),
        {propMode, markingMode, 3, numerator, 0, output});
  }
}

}  // namespace atlantis::testing
