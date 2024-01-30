#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/binaryMax.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BinaryMaxTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(solver->value(ts, inputs.at(0)),
                         solver->value(ts, inputs.at(1)));
  }

  static Int computeOutput(std::array<Int, 2> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId x, const VarId y) {
    return computeOutput(solver->value(ts, x), solver->value(ts, y));
  }

  static Int computeOutput(const Int xVal, const Int yVal) {
    return std::max(xVal, yVal);
  }
};

TEST_F(BinaryMaxTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  solver->open();
  const VarId x = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  BinaryMax& invariant =
      solver->makeInvariant<BinaryMax>(*solver, outputId, x, y);
  solver->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    solver->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      solver->updateBounds(y, yLb, yUb, false);
      solver->open();
      invariant.updateBounds(false);
      solver->close();
      EXPECT_EQ(solver->lowerBound(outputId), std::max(xLb, yLb));
      EXPECT_EQ(solver->upperBound(outputId), std::max(xUb, yUb));
    }
  }
}

TEST_F(BinaryMaxTest, Recompute) {
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 0;
  const Int yUb = 5;
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);

  solver->open();
  const VarId x = solver->makeIntVar(xUb, xLb, xUb);
  const VarId y = solver->makeIntVar(yUb, yLb, yUb);
  const VarId outputId =
      solver->makeIntVar(0, 0, std::max(xUb - yLb, yUb - xLb));
  BinaryMax& invariant =
      solver->makeInvariant<BinaryMax>(*solver, outputId, x, y);
  solver->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      solver->setValue(solver->currentTimestamp(), x, xVal);
      solver->setValue(solver->currentTimestamp(), y, yVal);

      const Int expectedOutput = computeOutput(xVal, yVal);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(BinaryMaxTest, NotifyInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::array<VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                              solver->makeIntVar(ub, lb, ub)};
  VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  BinaryMax& invariant = solver->makeInvariant<BinaryMax>(
      *solver, outputId, inputs.at(0), inputs.at(1));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      solver->setValue(ts, inputs.at(i), val);
      const Int expectedOutput = computeOutput(ts, inputs);

      invariant.notifyInputChanged(ts, LocalId(i));
      EXPECT_EQ(expectedOutput, solver->value(ts, outputId));
    }
  }
}

TEST_F(BinaryMaxTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  const std::array<VarId, 2> inputs = {solver->makeIntVar(lb, lb, ub),
                                       solver->makeIntVar(ub, lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  BinaryMax& invariant = solver->makeInvariant<BinaryMax>(
      *solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(BinaryMaxTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  EXPECT_TRUE(lb <= ub);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  BinaryMax& invariant = solver->makeInvariant<BinaryMax>(
      *solver, outputId, inputs.at(0), inputs.at(1));
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

TEST_F(BinaryMaxTest, Commit) {
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
  BinaryMax& invariant = solver->makeInvariant<BinaryMax>(
      *solver, outputId, inputs.at(0), inputs.at(1));
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

class MockBinaryMax : public BinaryMax {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BinaryMax::registerVars();
  }
  explicit MockBinaryMax(SolverBase& solver, VarId output, VarId x, VarId y)
      : BinaryMax(solver, output, x, y) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BinaryMax::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BinaryMax::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BinaryMax::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BinaryMax::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BinaryMax::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BinaryMaxTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId x = solver->makeIntVar(-10, -100, 100);
    const VarId y = solver->makeIntVar(10, -100, 100);
    const VarId output = solver->makeIntVar(0, 0, 200);
    testNotifications<MockBinaryMax>(
        &solver->makeInvariant<MockBinaryMax>(*solver, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace atlantis::testing
