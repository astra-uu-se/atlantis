#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/mod.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ModTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId x, const VarId y) {
    Int denominator = solver->value(ts, y);
    if (denominator == 0) {
      denominator = solver->upperBound(y) > 0 ? 1 : -1;
    }
    return solver->value(ts, x) % std::abs(denominator);
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

  solver->open();
  const VarId x = solver->makeIntVar(xUb, xLb, xUb);
  const VarId y = solver->makeIntVar(yUb, yLb, yUb);
  const VarId outputId = solver->makeIntVar(0, outputLb, outputUb);
  Mod& invariant = solver->makeInvariant<Mod>(*solver, outputId, x, y);
  solver->close();

  for (size_t i = 0; i < data.size(); ++i) {
    Timestamp ts = solver->currentTimestamp() + Timestamp(i + 1);
    const Int xVal = data.at(i).at(0);
    const Int yVal = data.at(i).at(1);
    const Int expectedOutput = data.at(i).at(2);

    solver->setValue(ts, x, xVal);
    solver->setValue(ts, y, yVal);

    invariant.recompute(ts);

    EXPECT_EQ(solver->value(ts, outputId), expectedOutput);
    EXPECT_EQ(computeOutput(ts, x, y), expectedOutput);
  }
}

TEST_F(ModTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  solver->open();
  const VarId x = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  Mod& invariant = solver->makeInvariant<Mod>(*solver, outputId, x, y);
  solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    solver->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      solver->updateBounds(y, yLb, yUb, false);
      solver->open();
      invariant.updateBounds();
      solver->close();
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        solver->setValue(solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          solver->setValue(solver->currentTimestamp(), y, yVal);
          invariant.recompute(solver->currentTimestamp());
          const Int o = solver->value(solver->currentTimestamp(), outputId);
          if (o < solver->lowerBound(outputId) ||
              solver->upperBound(outputId) < o) {
            ASSERT_GE(o, solver->lowerBound(outputId));
            ASSERT_LE(o, solver->upperBound(outputId));
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

  solver->open();
  const VarId x = solver->makeIntVar(xUb, xLb, xUb);
  const VarId y = solver->makeIntVar(yUb, yLb, yUb);
  const VarId outputId = solver->makeIntVar(outputLb, outputLb, outputUb);
  Mod& invariant = solver->makeInvariant<Mod>(*solver, outputId, x, y);
  solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      solver->setValue(solver->currentTimestamp(), x, xVal);
      solver->setValue(solver->currentTimestamp(), y, yVal);

      const Int expectedOutput =
          computeOutput(solver->currentTimestamp(), x, y);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  solver->open();
  std::array<VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                              solver->makeIntVar(ub, lb, ub)};
  VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  Mod& invariant =
      solver->makeInvariant<Mod>(*solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      solver->setValue(solver->currentTimestamp(), inputs.at(i), val);
      const Int expectedOutput =
          computeOutput(solver->currentTimestamp(), inputs);

      invariant.notifyInputChanged(solver->currentTimestamp(), LocalId(i));
      EXPECT_EQ(expectedOutput,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  solver->open();
  const std::array<VarId, 2> inputs = {solver->makeIntVar(lb, lb, ub),
                                       solver->makeIntVar(ub, lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  Mod& invariant =
      solver->makeInvariant<Mod>(*solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(ModTest, NotifyCurrentInputChanged) {
  const Int lb = -10002;
  const Int ub = -10000;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  Mod& invariant =
      solver->makeInvariant<Mod>(*solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
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

TEST_F(ModTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      solver->makeIntVar(committedValues.at(0), lb, ub),
      solver->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = solver->makeIntVar(0, 0, 2);
  Mod& invariant =
      solver->makeInvariant<Mod>(*solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(ModTest, ZeroDenominator) {
  const Int xVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [yLb, yUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, 0}, {-50, 50, 0}, {0, 100, 0}}) {
    EXPECT_TRUE(yLb <= yUb);
    EXPECT_TRUE(yLb != 0 || yUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      solver->open();
      const VarId x = solver->makeIntVar(xVal, xVal, xVal);
      const VarId y = solver->makeIntVar(0, yLb, yUb);
      const VarId outputId = solver->makeIntVar(0, outputLb, outputUb);
      Mod& invariant = solver->makeInvariant<Mod>(*solver, outputId, x, y);
      solver->close();

      EXPECT_EQ(expected, computeOutput(solver->currentTimestamp(), x, y));
      if (method == 0) {
        invariant.recompute(solver->currentTimestamp());
      } else {
        invariant.notifyInputChanged(solver->currentTimestamp(), LocalId(1));
      }
      EXPECT_EQ(expected, solver->value(solver->currentTimestamp(), outputId));
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
  explicit MockMod(SolverBase& solver, VarId output, VarId x, VarId y)
      : Mod(solver, output, x, y) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ModTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId x = solver->makeIntVar(-10, -100, 100);
    const VarId y = solver->makeIntVar(10, -100, 100);
    const VarId output = solver->makeIntVar(0, 0, 200);
    testNotifications<MockMod>(
        &solver->makeInvariant<MockMod>(*solver, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace atlantis::testing
