#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/pow.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class PowTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, const std::array<VarId, 2>& inputs) {
    return computeOutput(ts, inputs, 1);
  }
  Int computeOutput(Timestamp ts, const std::array<VarId, 2>& inputs,
                    Int zeroReplacement) {
    return computeOutput(solver->value(ts, inputs.at(0)),
                         solver->value(ts, inputs.at(1)), zeroReplacement);
  }

  static Int computeOutput(const std::array<Int, 2>& inputs) {
    return computeOutput(inputs.at(0), inputs.at(1), 1);
  }

  static Int computeOutput(const std::array<Int, 2>& inputs, Int zeroReplacement) {
    return computeOutput(inputs.at(0), inputs.at(1), zeroReplacement);
  }

  Int computeOutput(Timestamp ts, const VarId base, const VarId exponent) {
    return computeOutput(ts, base, exponent, 1);
  }

  Int computeOutput(Timestamp ts, const VarId base, const VarId exponent,
                    Int zeroReplacement) {
    return computeOutput(solver->value(ts, base), solver->value(ts, exponent),
                         zeroReplacement);
  }

  static Int computeOutput(const Int baseVal, const Int expVal) {
    return computeOutput(baseVal, expVal, 1);
  }

  static Int computeOutput(const Int baseVal, const Int expVal, Int zeroReplacement) {
    if (baseVal == 0 && expVal < 0) {
      return static_cast<Int>(std::pow(zeroReplacement, expVal));
    }
    return static_cast<Int>(std::pow(baseVal, expVal));
  }
};

TEST_F(PowTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-8, -5}, {-3, 0}, {-2, 2}, {0, 3}, {5, 8}};
  solver->open();
  const VarId base = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId exponent = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = solver->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Pow& invariant = solver->makeInvariant<Pow>(*solver, outputId, base, exponent);
  solver->close();

  for (const auto& [baseLb, baseUb] : boundVec) {
    EXPECT_TRUE(baseLb <= baseUb);
    solver->updateBounds(base, baseLb, baseUb, false);
    for (const auto& [expLb, expUb] : boundVec) {
      EXPECT_TRUE(expLb <= expUb);
      solver->updateBounds(exponent, expLb, expUb, false);
      solver->open();
      solver->close();
      for (Int baseVal = baseLb; baseVal <= baseUb; ++baseVal) {
        solver->setValue(solver->currentTimestamp(), base, baseVal);
        for (Int expVal = expLb; expVal <= expUb; ++expVal) {
          solver->setValue(solver->currentTimestamp(), exponent, expVal);
          invariant.recompute(solver->currentTimestamp());
          const Int o = solver->value(solver->currentTimestamp(), outputId);
          if (o < solver->lowerBound(outputId) ||
              solver->upperBound(outputId) < o) {
            invariant.updateBounds(false);
            ASSERT_GE(o, solver->lowerBound(outputId));
            ASSERT_LE(o, solver->upperBound(outputId));
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

  solver->open();
  const VarId base = solver->makeIntVar(baseUb, baseLb, baseUb);
  const VarId exponent = solver->makeIntVar(expUb, expLb, expUb);
  const VarId outputId =
      solver->makeIntVar(0, 0, std::max(baseUb - expLb, expUb - baseLb));
  Pow& invariant = solver->makeInvariant<Pow>(*solver, outputId, base, exponent);
  solver->close();

  for (Int baseVal = baseLb; baseVal <= baseUb; ++baseVal) {
    for (Int expVal = expLb; expVal <= expUb; ++expVal) {
      solver->setValue(solver->currentTimestamp(), base, baseVal);
      solver->setValue(solver->currentTimestamp(), exponent, expVal);

      const Int expectedOutput = computeOutput(baseVal, expVal);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(PowTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::array<VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                              solver->makeIntVar(ub, lb, ub)};
  VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  Pow& invariant =
      solver->makeInvariant<Pow>(*solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      solver->setValue(ts, inputs.at(i), val);
      const Int expectedOutput = computeOutput(ts, inputs, 1);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, solver->value(ts, outputId));
    }
  }
}

TEST_F(PowTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<VarId, 2> inputs = {solver->makeIntVar(lb, lb, ub),
                                       solver->makeIntVar(ub, lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  Pow& invariant =
      solver->makeInvariant<Pow>(*solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
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

TEST_F(PowTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  Pow& invariant =
      solver->makeInvariant<Pow>(*solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (const VarId& varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = solver->value(ts, varId);
      do {
        solver->setValue(ts, varId, valueDist(gen));
      } while (solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(PowTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      solver->makeIntVar(committedValues.at(0), lb, ub),
      solver->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = solver->makeIntVar(0, 0, 2);
  Pow& invariant =
      solver->makeInvariant<Pow>(*solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  EXPECT_EQ(solver->value(solver->currentTimestamp(), outputId),
            computeOutput(solver->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputs.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      solver->setValue(ts, inputs.at(i), valueDist(gen));
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

class MockPow : public Pow {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Pow::registerVars();
  }
  explicit MockPow(SolverBase& solver, VarId output, VarId base, VarId exponent)
      : Pow(solver, output, base, exponent) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(PowTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId base = solver->makeIntVar(-10, -100, 100);
    const VarId exponent = solver->makeIntVar(10, -100, 100);
    const VarId output = solver->makeIntVar(0, 0, 200);
    testNotifications<MockPow>(
        &solver->makeInvariant<MockPow>(*solver, output, base, exponent),
        {propMode, markingMode, 3, base, 0, output});
  }
}

}  // namespace atlantis::testing
