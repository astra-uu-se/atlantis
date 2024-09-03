#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/count.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class CountTest : public InvariantTest {
 public:
  Int computeOutput(const Timestamp ts, const VarId y,
                    const std::vector<VarId>& vars) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = _solver->value(ts, vars.at(i));
    }
    return computeOutput(_solver->value(ts, y), values);
  }

  static Int computeOutput(const Int y, const std::vector<Int>& values) {
    Int count = 0;
    for (Int value : values) {
      if (value == y) {
        ++count;
      }
    }
    return count;
  }
};

TEST_F(CountTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, 0}, {-5, 5}, {0, 10}, {15, 20}};
  _solver->open();

  const VarId y = _solver->makeIntVar(0, 0, 10);
  std::vector<VarId> vars{_solver->makeIntVar(0, 0, 10),
                          _solver->makeIntVar(0, 0, 10),
                          _solver->makeIntVar(0, 0, 10)};
  const VarId outputId = _solver->makeIntVar(0, 0, 2);
  Count& invariant = _solver->makeInvariant<Count>(*_solver, outputId, y,
                                                   std::vector<VarId>(vars));

  for (const auto& [yLb, yUb] : boundVec) {
    EXPECT_TRUE(yLb <= yUb);
    _solver->updateBounds(y, yLb, yUb, false);
    for (const auto& [aLb, aUb] : boundVec) {
      EXPECT_TRUE(aLb <= aUb);
      _solver->updateBounds(vars.at(0), aLb, aUb, false);
      for (const auto& [bLb, bUb] : boundVec) {
        EXPECT_TRUE(bLb <= bUb);
        _solver->updateBounds(vars.at(1), bLb, bUb, false);
        for (const auto& [cLb, cUb] : boundVec) {
          EXPECT_TRUE(cLb <= cUb);
          _solver->updateBounds(vars.at(2), cLb, cUb, false);
          invariant.updateBounds(false);

          ASSERT_GE(0, _solver->lowerBound(outputId));
          ASSERT_LE(vars.size(), _solver->upperBound(outputId));
        }
      }
    }
  }
}

TEST_F(CountTest, Recompute) {
  const Int lb = -5;
  const Int ub = 5;

  ASSERT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> dist(lb, ub);

  _solver->open();

  const VarId y = _solver->makeIntVar(dist(gen), lb, ub);
  const VarId a = _solver->makeIntVar(dist(gen), lb, ub);
  const VarId b = _solver->makeIntVar(dist(gen), lb, ub);
  const VarId c = _solver->makeIntVar(dist(gen), lb, ub);

  std::vector<VarId> inputs{a, b, c};

  const VarId outputId = _solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                             std::numeric_limits<Int>::max());

  Count& invariant = _solver->makeInvariant<Count>(*_solver, outputId, y,
                                                   std::vector<VarId>(inputs));
  _solver->close();

  for (Int yVal = lb; yVal <= ub; ++yVal) {
    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          _solver->setValue(_solver->currentTimestamp(), y, yVal);
          _solver->setValue(_solver->currentTimestamp(), a, aVal);
          _solver->setValue(_solver->currentTimestamp(), b, bVal);
          _solver->setValue(_solver->currentTimestamp(), c, cVal);
          const Int expectedOutput =
              computeOutput(_solver->currentTimestamp(), y, inputs);
          invariant.recompute(_solver->currentTimestamp());
          EXPECT_EQ(expectedOutput,
                    _solver->value(_solver->currentTimestamp(), outputId));
        }
      }
    }
  }
}

TEST_F(CountTest, NotifyInputChanged) {
  _solver->open();
  const size_t numInputs = 3;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  const VarId y = _solver->makeIntVar(dist(gen), lb, ub);
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
  }
  const VarId outputId = _solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                             std::numeric_limits<Int>::max());
  Count& invariant = _solver->makeInvariant<Count>(*_solver, outputId, y,
                                                   std::vector<VarId>(inputs));
  _solver->close();

  const Timestamp ts = _solver->currentTimestamp() + 1;

  std::vector<VarId> allInputs(inputs);
  allInputs.emplace_back(y);

  for (size_t i = 0; i < allInputs.size(); ++i) {
    const Int oldVal = _solver->value(ts, allInputs.at(i));
    do {
      _solver->setValue(ts, allInputs.at(i), dist(gen));
    } while (oldVal == _solver->value(ts, allInputs.at(i)));

    const Int expectedOutput = computeOutput(ts, y, inputs);

    invariant.notifyInputChanged(ts, LocalId(i));
    EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
  }
}

TEST_F(CountTest, NextInput) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);

  _solver->open();
  const VarId y = _solver->makeIntVar(dist(gen), lb, ub);
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
  }
  const VarId outputId = _solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                             std::numeric_limits<Int>::max());
  Count& invariant = _solver->makeInvariant<Count>(*_solver, outputId, y,
                                                   std::vector<VarId>(inputs));
  _solver->close();

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId minVarId =
      std::min(y, *std::min_element(inputs.begin(), inputs.end()));
  const VarId maxVarId =
      std::max(y, *std::max_element(inputs.begin(), inputs.end()));

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i <= numInputs; ++i) {
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

TEST_F(CountTest, NotifyCurrentInputChanged) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  _solver->open();
  const VarId y = _solver->makeIntVar(dist(gen), lb, ub);
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
  }

  const VarId outputId = _solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                             std::numeric_limits<Int>::max());
  Count& invariant = _solver->makeInvariant<Count>(*_solver, outputId, y,
                                                   std::vector<VarId>(inputs));
  _solver->close();

  std::vector<VarId> allInputs(inputs);
  allInputs.emplace_back(y);

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarId& varId : allInputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, dist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputId), computeOutput(ts, y, inputs));
    }
  }
}

TEST_F(CountTest, Commit) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);
  std::vector<VarId> inputs(numInputs, NULL_ID);
  std::vector<size_t> indices(numInputs + 1, 0);
  std::vector<Int> committedValues(numInputs + 1, 0);

  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    committedValues.at(i) = dist(gen);
    inputs.at(i) = _solver->makeIntVar(committedValues.at(i), lb, ub);
  }
  indices.back() = numInputs;
  committedValues.back() = dist(gen);
  const VarId y = _solver->makeIntVar(committedValues.back(), lb, ub);

  const VarId outputId = _solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                             std::numeric_limits<Int>::max());
  Count& invariant = _solver->makeInvariant<Count>(*_solver, outputId, y,
                                                   std::vector<VarId>(inputs));

  std::shuffle(indices.begin(), indices.end(), rng);

  _solver->close();
  std::vector<VarId> allInputs(inputs);
  allInputs.emplace_back(y);

  EXPECT_EQ(_solver->value(_solver->currentTimestamp(), outputId),
            computeOutput(_solver->currentTimestamp(), y, inputs));

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(allInputs.at(j)),
                committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, allInputs.at(i), dist(gen));
    } while (oldVal == _solver->value(ts, allInputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = _solver->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

    _solver->commitIf(ts, allInputs.at(i));
    committedValues.at(i) = _solver->value(ts, allInputs.at(i));
    _solver->commitIf(ts, outputId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputId));
  }
}

class MockCount : public Count {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Count::registerVars();
  }
  explicit MockCount(SolverBase& _solver, VarId output, VarId y,
                     std::vector<VarId>&& varArray)
      : Count(_solver, output, y, std::move(varArray)) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Count::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Count::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Count::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Count::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Count::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(CountTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const size_t numArgs = 10;
    const VarId y = _solver->makeIntVar(0, 0, numArgs);
    std::vector<VarId> args;
    for (size_t value = 1; value <= numArgs; ++value) {
      args.push_back(_solver->makeIntVar(static_cast<Int>(value), 1,
                                         static_cast<Int>(numArgs)));
    }
    const VarId modifiedVarId = args.front();
    const VarId output = _solver->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCount>(
        &_solver->makeInvariant<MockCount>(*_solver, output, y,
                                           std::move(args)),
        {propMode, markingMode, numArgs + 2, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
