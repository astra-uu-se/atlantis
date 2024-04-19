#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/ifThenElse.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IfThenElseTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarViewId, 3> inputs) {
    return computeOutput(_solver->value(ts, inputs.at(0)),
                         _solver->value(ts, inputs.at(1)),
                         _solver->value(ts, inputs.at(2)));
  }

  static Int computeOutput(std::array<Int, 3> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1), inputs.at(2));
  }

  Int computeOutput(Timestamp ts, const VarViewId b, const VarViewId x,
                    const VarViewId y) {
    return computeOutput(_solver->value(ts, b), _solver->value(ts, x),
                         _solver->value(ts, y));
  }

  static Int computeOutput(const Int bVal, const Int xVal, const Int yVal) {
    return bVal == 0 ? xVal : yVal;
  }
};

TEST_F(IfThenElseTest, UpdateBounds) {
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 100;
  const Int yUb = 1000;
  EXPECT_TRUE(xLb <= xUb);

  _solver->open();
  const VarViewId b = _solver->makeIntVar(0, 0, 10);
  const VarViewId x = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId y = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId outputId =
      _solver->makeIntVar(0, std::min(xLb, yLb), std::max(xUb, yUb));
  IfThenElse& invariant =
      _solver->makeInvariant<IfThenElse>(*_solver, outputId, b, x, y);
  _solver->close();

  std::vector<std::pair<Int, Int>> bBounds{{0, 0}, {0, 100}, {1, 10000}};

  for (const auto& [bLb, bUb] : bBounds) {
    EXPECT_TRUE(bLb <= bUb);
    _solver->updateBounds(VarId(b), bLb, bUb, false);
    invariant.updateBounds(false);
    if (bLb == 0 && bUb == 0) {
      EXPECT_EQ(_solver->lowerBound(outputId), _solver->lowerBound(x));
      EXPECT_EQ(_solver->upperBound(outputId), _solver->upperBound(x));
    } else if (bLb > 0) {
      EXPECT_EQ(_solver->lowerBound(outputId), _solver->lowerBound(y));
      EXPECT_EQ(_solver->upperBound(outputId), _solver->upperBound(y));
    } else {
      EXPECT_EQ(_solver->lowerBound(outputId),
                std::max(_solver->lowerBound(x), _solver->lowerBound(y)));
      EXPECT_EQ(_solver->upperBound(outputId),
                std::min(_solver->upperBound(x), _solver->upperBound(y)));
    }
  }
}

TEST_F(IfThenElseTest, Recompute) {
  const Int bLb = 0;
  const Int bUb = 5;
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 0;
  const Int yUb = 5;
  EXPECT_TRUE(bLb <= bUb);
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  _solver->open();
  const VarViewId b = _solver->makeIntVar(bLb, bLb, bUb);
  const VarViewId x = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId y = _solver->makeIntVar(yUb, yLb, yUb);
  const VarViewId outputId =
      _solver->makeIntVar(0, std::min(xLb, yLb), std::max(xUb, yUb));
  IfThenElse& invariant =
      _solver->makeInvariant<IfThenElse>(*_solver, outputId, b, x, y);
  _solver->close();
  for (Int bVal = bLb; bVal <= bUb; ++bVal) {
    for (Int xVal = xLb; xVal <= xUb; ++xVal) {
      for (Int yVal = yLb; yVal <= yUb; ++yVal) {
        _solver->setValue(_solver->currentTimestamp(), b, bVal);
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        _solver->setValue(_solver->currentTimestamp(), y, yVal);

        const Int expectedOutput = computeOutput(bVal, xVal, yVal);
        invariant.recompute(_solver->currentTimestamp());
        EXPECT_EQ(expectedOutput,
                  _solver->value(_solver->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(IfThenElseTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  _solver->open();
  std::array<VarViewId, 3> inputs{_solver->makeIntVar(bLb, bLb, bUb),
                                  _solver->makeIntVar(ub, lb, ub),
                                  _solver->makeIntVar(ub, lb, ub)};
  VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  IfThenElse& invariant = _solver->makeInvariant<IfThenElse>(
      *_solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
  _solver->close();

  Timestamp ts = _solver->currentTimestamp();

  for (Int bVal = bLb; bVal <= bUb; ++bVal) {
    for (Int val = lb; val <= ub; ++val) {
      for (size_t i = 1; i < inputs.size(); ++i) {
        ++ts;
        _solver->setValue(ts, inputs.at(0), bVal);
        _solver->setValue(ts, inputs.at(i), val);
        const Int expectedOutput = computeOutput(ts, inputs);

        invariant.notifyInputChanged(ts, LocalId(i));
        EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
      }
    }
  }
}

TEST_F(IfThenElseTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  _solver->open();
  const std::array<VarViewId, 3> inputs = {_solver->makeIntVar(bLb, bLb, bUb),
                                           _solver->makeIntVar(lb, lb, ub),
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
  IfThenElse& invariant = _solver->makeInvariant<IfThenElse>(
      *_solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(size_t(maxVarId) + 1, false);
    // First input is b,
    // Second input is x if b = 0, otherwise y:
    for (size_t i = 0; i < 2; ++i) {
      const VarViewId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_LE(size_t(minVarId), size_t(varId));
      EXPECT_GE(size_t(maxVarId), size_t(varId));
      EXPECT_FALSE(notified.at(size_t(varId)));
      notified.at(size_t(varId)) = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    const Int bVal = _solver->value(ts, inputs.at(0));

    EXPECT_TRUE(notified.at(size_t(inputs.at(0))));
    EXPECT_TRUE(notified.at(size_t(inputs.at(bVal == 0 ? 1 : 2))));
    EXPECT_FALSE(notified.at(size_t(inputs.at(bVal == 0 ? 2 : 1))));
  }
}

TEST_F(IfThenElseTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  _solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<Int> bDist(bLb, bUb);

  const std::array<VarViewId, 3> inputs = {
      _solver->makeIntVar(bLb, bLb, bLb),
      _solver->makeIntVar(valueDist(gen), lb, ub),
      _solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarViewId outputId = _solver->makeIntVar(0, 0, ub - lb);
  IfThenElse& invariant = _solver->makeInvariant<IfThenElse>(
      *_solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (size_t i = 0; i < 2; ++i) {
      const Int bOld = _solver->value(ts, inputs.at(0));
      const VarViewId curInput = invariant.nextInput(ts);
      EXPECT_EQ(curInput, inputs.at(i == 0 ? 0 : bOld == 0 ? 1 : 2));

      const Int oldVal = _solver->value(ts, curInput);
      do {
        _solver->setValue(ts, curInput, i == 0 ? bDist(gen) : valueDist(gen));
      } while (_solver->value(ts, curInput) == oldVal);

      invariant.notifyCurrentInputChanged(ts);

      EXPECT_EQ(_solver->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(IfThenElseTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  _solver->open();
  std::uniform_int_distribution<Int> bDist(bLb, bUb);
  std::uniform_int_distribution<Int> valueDist(lb, ub);

  std::array<size_t, 3> indices{0, 1, 2};
  std::array<Int, 3> committedValues{bDist(gen), valueDist(gen),
                                     valueDist(gen)};
  std::array<VarViewId, 3> inputs{
      _solver->makeIntVar(committedValues.at(0), bLb, bUb),
      _solver->makeIntVar(committedValues.at(1), lb, ub),
      _solver->makeIntVar(committedValues.at(2), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  IfThenElse& invariant = _solver->makeInvariant<IfThenElse>(
      *_solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
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
      _solver->setValue(ts, inputs.at(i), i == 0 ? bDist(gen) : valueDist(gen));
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

class MockIfThenElse : public IfThenElse {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    IfThenElse::registerVars();
  }
  explicit MockIfThenElse(SolverBase& solver, VarViewId output, VarViewId b,
                          VarViewId x, VarViewId y)
      : IfThenElse(solver, output, b, x, y) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return IfThenElse::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return IfThenElse::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          IfThenElse::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          IfThenElse::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      IfThenElse::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(IfThenElseTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId b = _solver->makeIntVar(0, -100, 100);
    const VarViewId x = _solver->makeIntVar(0, 0, 4);
    const VarViewId y = _solver->makeIntVar(5, 5, 9);
    const VarViewId output = _solver->makeIntVar(3, 0, 9);
    testNotifications<MockIfThenElse>(
        &_solver->makeInvariant<MockIfThenElse>(*_solver, output, b, x, y),
        {propMode, markingMode, 3, b, 5, output});
  }
}

}  // namespace atlantis::testing
