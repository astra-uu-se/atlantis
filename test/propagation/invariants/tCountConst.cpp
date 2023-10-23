#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/countConst.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class CountConstTest : public InvariantTest {
 public:
  Int computeOutput(const Timestamp ts, const Int y,
                    const std::vector<VarId>& variables) {
    std::vector<Int> values(variables.size(), 0);
    for (size_t i = 0; i < variables.size(); ++i) {
      values.at(i) = solver->value(ts, variables.at(i));
    }
    return computeOutput(y, values);
  }

  Int computeOutput(const Int y, const std::vector<Int>& values) {
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
  solver->open();

  const Int y = 10;
  std::vector<VarId> inputs{solver->makeIntVar(0, 0, 10),
                            solver->makeIntVar(0, 0, 10),
                            solver->makeIntVar(0, 0, 10)};
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  CountConst& invariant =
      solver->makeInvariant<CountConst>(*solver, outputId, y, inputs);

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_TRUE(aLb <= aUb);
    solver->updateBounds(inputs.at(0), aLb, aUb, false);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_TRUE(bLb <= bUb);
      solver->updateBounds(inputs.at(1), bLb, bUb, false);
      for (const auto& [cLb, cUb] : boundVec) {
        EXPECT_TRUE(cLb <= cUb);
        solver->updateBounds(inputs.at(2), cLb, cUb, false);
        invariant.updateBounds();

        ASSERT_GE(0, solver->lowerBound(outputId));
        ASSERT_LE(inputs.size(), solver->upperBound(outputId));
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
    solver->open();

    const VarId a = solver->makeIntVar(dist(gen), lb, ub);
    const VarId b = solver->makeIntVar(dist(gen), lb, ub);
    const VarId c = solver->makeIntVar(dist(gen), lb, ub);

    std::vector<VarId> inputs{a, b, c};

    const VarId outputId = solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

    CountConst& invariant =
        solver->makeInvariant<CountConst>(*solver, outputId, y, inputs);
    solver->close();

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          solver->setValue(solver->currentTimestamp(), a, aVal);
          solver->setValue(solver->currentTimestamp(), b, bVal);
          solver->setValue(solver->currentTimestamp(), c, cVal);
          const Int expectedOutput =
              computeOutput(solver->currentTimestamp(), y, inputs);
          invariant.recompute(solver->currentTimestamp());
          EXPECT_EQ(expectedOutput,
                    solver->value(solver->currentTimestamp(), outputId));
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

  const Timestamp ts = solver->currentTimestamp() + (ub - lb) + 2;

  for (Int y = lb; y <= ub; ++y) {
    EXPECT_NE(ts, solver->currentTimestamp());
    solver->open();
    std::vector<VarId> inputs(numInputs, NULL_ID);
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.at(i) = solver->makeIntVar(dist(gen), lb, ub);
    }
    const VarId outputId = solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant =
        solver->makeInvariant<CountConst>(*solver, outputId, y, inputs);
    solver->close();
    EXPECT_NE(ts, solver->currentTimestamp());

    for (size_t i = 0; i < inputs.size(); ++i) {
      const Int oldVal = solver->value(ts, inputs.at(i));
      do {
        solver->setValue(ts, inputs.at(i), dist(gen));
      } while (oldVal == solver->value(ts, inputs.at(i)));

      const Int expectedOutput = computeOutput(ts, y, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, solver->value(ts, outputId));
    }
  }
}

TEST_F(CountConstTest, NextInput) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  const Int y = 0;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);

  solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = solver->makeIntVar(dist(gen), lb, ub);
  }
  const VarId outputId = solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  CountConst& invariant =
      solver->makeInvariant<CountConst>(*solver, outputId, y, inputs);
  solver->close();

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
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

TEST_F(CountConstTest, NotifyCurrentInputChanged) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = solver->makeIntVar(dist(gen), lb, ub);
  }

  for (Int y = lb; y <= ub; ++y) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId outputId = solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant =
        solver->makeInvariant<CountConst>(*solver, outputId, y, inputs);
    solver->close();

    for (Timestamp ts = solver->currentTimestamp() + 1;
         ts < solver->currentTimestamp() + 4; ++ts) {
      for (const VarId varId : inputs) {
        EXPECT_EQ(invariant.nextInput(ts), varId);
        const Int oldVal = solver->value(ts, varId);
        do {
          solver->setValue(ts, varId, dist(gen));
        } while (solver->value(ts, varId) == oldVal);
        invariant.notifyCurrentInputChanged(ts);
        EXPECT_EQ(solver->value(ts, outputId), computeOutput(ts, y, inputs));
      }
    }
  }
}

TEST_F(CountConstTest, Commit) {
  const size_t numInputs = 100;
  const Int lb = -10;
  const Int ub = 10;
  std::uniform_int_distribution<Int> dist(lb, ub);

  std::vector<VarId> inputs(numInputs, NULL_ID);
  std::vector<size_t> indices(numInputs, 0);
  std::vector<Int> committedValues(numInputs, 0);

  solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    inputs.at(i) = solver->makeIntVar(dist(gen), lb, ub);
  }

  for (Int y = lb; y <= ub; ++y) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId outputId = solver->makeIntVar(
        0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
    CountConst& invariant =
        solver->makeInvariant<CountConst>(*solver, outputId, y, inputs);

    solver->close();

    for (size_t i = 0; i < numInputs; ++i) {
      committedValues.at(i) = solver->committedValue(inputs.at(i));
    }

    std::shuffle(indices.begin(), indices.end(), rng);

    EXPECT_EQ(solver->value(solver->currentTimestamp(), outputId),
              computeOutput(solver->currentTimestamp(), y, inputs));

    for (const size_t i : indices) {
      Timestamp ts = solver->currentTimestamp() + Timestamp(i);
      for (size_t j = 0; j < numInputs; ++j) {
        // Check that we do not accidentally commit:
        ASSERT_EQ(solver->committedValue(inputs.at(j)), committedValues.at(j));
      }

      const Int oldVal = committedValues.at(i);
      do {
        solver->setValue(ts, inputs.at(i), dist(gen));
      } while (oldVal == solver->value(ts, inputs.at(i)));

      // notify changes
      invariant.notifyInputChanged(ts, LocalId(i));

      // incremental value
      const Int notifiedOutput = solver->value(ts, outputId);
      invariant.recompute(ts);

      ASSERT_EQ(notifiedOutput, solver->value(ts, outputId));

      solver->commitIf(ts, inputs.at(i));
      committedValues.at(i) = solver->value(ts, inputs.at(i));
      solver->commitIf(ts, outputId);

      invariant.commit(ts);
      invariant.recompute(ts + 1);
      ASSERT_EQ(notifiedOutput, solver->value(ts + 1, outputId));
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
  explicit MockCountConst(SolverBase& solver, VarId output, Int y,
                          const std::vector<VarId>& varArray)
      : CountConst(solver, output, y, varArray) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(CountConstTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const size_t numArgs = 10;
    const Int y = 5;
    std::vector<VarId> varArray;
    for (size_t value = 1; value <= numArgs; ++value) {
      varArray.push_back(solver->makeIntVar(static_cast<Int>(value), 1,
                                            static_cast<Int>(numArgs)));
    }
    const VarId modifiedVarId = varArray.front();
    const VarId output = solver->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCountConst>(
        &solver->makeInvariant<MockCountConst>(*solver, output, y, varArray),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
