#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class CountConstTest : public InvariantTest {
 public:
  Int computeOutput(const Timestamp ts, const Int y,
                    const std::vector<VarViewId>& vars) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = _solver->value(ts, vars.at(i));
    }
    return computeOutput(y, values);
  }

  static Int computeOutput(const Int y, const std::vector<Int>& values) {
    Int occurrences = 0;
    for (const Int val : values) {
      occurrences += (val == y ? 1 : 0);
    }
    return occurrences;
  }
};

TEST_F(CountConstTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, 0}, {-5, 5}, {0, 10}, {15, 20}};
  _solver->open();

  const Int y = 10;
  std::vector<VarViewId> inputs{_solver->makeIntVar(0, 0, 10),
                                _solver->makeIntVar(0, 0, 10),
                                _solver->makeIntVar(0, 0, 10)};
  const VarViewId outputId = _solver->makeIntVar(0, 0, 2);
  CountConst& invariant = _solver->makeInvariant<CountConst>(
      *_solver, outputId, y, std::vector<VarViewId>(inputs));

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_TRUE(aLb <= aUb);
    _solver->updateBounds(VarId(inputs.at(0)), aLb, aUb, false);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_TRUE(bLb <= bUb);
      _solver->updateBounds(VarId(inputs.at(1)), bLb, bUb, false);
      for (const auto& [cLb, cUb] : boundVec) {
        EXPECT_TRUE(cLb <= cUb);
        _solver->updateBounds(VarId(inputs.at(2)), cLb, cUb, false);
        invariant.updateBounds(false);

        ASSERT_GE(0, _solver->lowerBound(outputId));
        ASSERT_LE(inputs.size(), _solver->upperBound(outputId));
      }
    }
  }
}

TEST_F(CountConstTest, Recompute) {
  const Int lb = -5;
  const Int ub = 5;

  ASSERT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> dist(lb, ub);

  for (Int y = lb; y <= ub; ++y) {
    _solver->open();

    const VarViewId a = _solver->makeIntVar(dist(gen), lb, ub);
    const VarViewId b = _solver->makeIntVar(dist(gen), lb, ub);
    const VarViewId c = _solver->makeIntVar(dist(gen), lb, ub);

    std::vector<VarViewId> inputs{a, b, c};

    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

    CountConst& invariant = _solver->makeInvariant<CountConst>(
        *_solver, outputId, y, std::vector<VarViewId>(inputs));
    _solver->close();

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
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

TEST_F(CountConstTest, NotifyInputChanged) {
  const size_t numInputs = 3;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  const Timestamp ts = _solver->currentTimestamp() + (ub - lb) + 2;

  for (Int y = lb; y <= ub; ++y) {
    EXPECT_NE(ts, _solver->currentTimestamp());
    _solver->open();
    std::vector<VarViewId> inputs(numInputs, NULL_ID);
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
    }
    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant = _solver->makeInvariant<CountConst>(
        *_solver, outputId, y, std::vector<VarViewId>(inputs));
    _solver->close();
    EXPECT_NE(ts, _solver->currentTimestamp());

    for (size_t i = 0; i < inputs.size(); ++i) {
      const Int oldVal = _solver->value(ts, VarId(inputs.at(i)));
      do {
        _solver->setValue(ts, inputs.at(i), dist(gen));
      } while (oldVal == _solver->value(ts, inputs.at(i)));

      const Int expectedOutput = computeOutput(ts, y, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
    }
  }
}

TEST_F(CountConstTest, NextInput) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  const Int y = 0;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarViewId> inputs(numInputs, NULL_ID);

  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
  }
  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  CountConst& invariant = _solver->makeInvariant<CountConst>(
      *_solver, outputId, y, std::vector<VarViewId>(inputs));
  _solver->close();

  std::shuffle(inputs.begin(), inputs.end(), rng);

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

TEST_F(CountConstTest, NotifyCurrentInputChanged) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarViewId> inputs(numInputs, NULL_ID);
  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
  }

  for (Int y = lb; y <= ub; ++y) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant = _solver->makeInvariant<CountConst>(
        *_solver, outputId, y, std::vector<VarViewId>(inputs));
    _solver->close();

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      for (const VarViewId& varId : inputs) {
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
}

TEST_F(CountConstTest, Commit) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarViewId> inputs(numInputs, NULL_ID);
  std::vector<size_t> indices(numInputs, 0);
  std::vector<Int> committedValues(numInputs, 0);

  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    inputs.at(i) = _solver->makeIntVar(dist(gen), lb, ub);
  }

  for (Int y = lb; y <= ub; ++y) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId outputId = _solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant = _solver->makeInvariant<CountConst>(
        *_solver, outputId, y, std::vector<VarViewId>(inputs));

    _solver->close();

    for (size_t i = 0; i < numInputs; ++i) {
      committedValues.at(i) = _solver->committedValue(inputs.at(i));
    }

    std::shuffle(indices.begin(), indices.end(), rng);

    EXPECT_EQ(_solver->value(_solver->currentTimestamp(), outputId),
              computeOutput(_solver->currentTimestamp(), y, inputs));

    for (const size_t i : indices) {
      Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
      for (size_t j = 0; j < numInputs; ++j) {
        // Check that we do not accidentally commit:
        ASSERT_EQ(_solver->committedValue(inputs.at(j)), committedValues.at(j));
      }

      const Int oldVal = committedValues.at(i);
      do {
        _solver->setValue(ts, inputs.at(i), dist(gen));
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
}

class MockCountConst : public CountConst {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    CountConst::registerVars();
  }
  explicit MockCountConst(SolverBase& solver, VarViewId output, Int y,
                          std::vector<VarViewId>&& varArray)
      : CountConst(solver, output, y, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return CountConst::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return CountConst::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          CountConst::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          CountConst::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      CountConst::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(CountConstTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const size_t numArgs = 10;
    const Int y = 5;
    std::vector<VarViewId> varArray;
    for (size_t value = 1; value <= numArgs; ++value) {
      varArray.push_back(_solver->makeIntVar(static_cast<Int>(value), 1,
                                             static_cast<Int>(numArgs)));
    }
    const VarViewId modifiedVarId = varArray.front();
    const VarViewId output = _solver->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCountConst>(
        &_solver->makeInvariant<MockCountConst>(*_solver, output, y,
                                                std::move(varArray)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
