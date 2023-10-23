
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/ifThenElse.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IfThenElseTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 3> inputs) {
    return computeOutput(solver->value(ts, inputs.at(0)),
                         solver->value(ts, inputs.at(1)),
                         solver->value(ts, inputs.at(2)));
  }

  Int computeOutput(std::array<Int, 3> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1), inputs.at(2));
  }

  Int computeOutput(Timestamp ts, const VarId b, const VarId x, const VarId y) {
    return computeOutput(solver->value(ts, b), solver->value(ts, x),
                         solver->value(ts, y));
  }

  Int computeOutput(const Int bVal, const Int xVal, const Int yVal) {
    return bVal == 0 ? xVal : yVal;
  }
};

TEST_F(IfThenElseTest, UpdateBounds) {
  const Int xLb = 0;
  const Int xUb = 10;
  const Int yLb = 100;
  const Int yUb = 1000;
  EXPECT_TRUE(xLb <= xUb);

  solver->open();
  const VarId b = solver->makeIntVar(0, 0, 10);
  const VarId x = solver->makeIntVar(yUb, yLb, yUb);
  const VarId y = solver->makeIntVar(yUb, yLb, yUb);
  const VarId outputId =
      solver->makeIntVar(0, std::min(xLb, yLb), std::max(xUb, yUb));
  IfThenElse& invariant =
      solver->makeInvariant<IfThenElse>(*solver, outputId, b, x, y);
  solver->close();

  std::vector<std::pair<Int, Int>> bBounds{{0, 0}, {0, 100}, {1, 10000}};

  for (const auto& [bLb, bUb] : bBounds) {
    EXPECT_TRUE(bLb <= bUb);
    solver->updateBounds(b, bLb, bUb, false);
    invariant.updateBounds();
    if (bLb == 0 && bUb == 0) {
      EXPECT_EQ(solver->lowerBound(outputId), solver->lowerBound(x));
      EXPECT_EQ(solver->upperBound(outputId), solver->upperBound(x));
    } else if (bLb > 0) {
      EXPECT_EQ(solver->lowerBound(outputId), solver->lowerBound(y));
      EXPECT_EQ(solver->upperBound(outputId), solver->upperBound(y));
    } else {
      EXPECT_EQ(solver->lowerBound(outputId),
                std::max(solver->lowerBound(x), solver->lowerBound(y)));
      EXPECT_EQ(solver->upperBound(outputId),
                std::min(solver->upperBound(x), solver->upperBound(y)));
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

  solver->open();
  const VarId b = solver->makeIntVar(bLb, bLb, bUb);
  const VarId x = solver->makeIntVar(yUb, yLb, yUb);
  const VarId y = solver->makeIntVar(yUb, yLb, yUb);
  const VarId outputId =
      solver->makeIntVar(0, std::min(xLb, yLb), std::max(xUb, yUb));
  IfThenElse& invariant =
      solver->makeInvariant<IfThenElse>(*solver, outputId, b, x, y);
  solver->close();
  for (Int bVal = bLb; bVal <= bUb; ++bVal) {
    for (Int xVal = xLb; xVal <= xUb; ++xVal) {
      for (Int yVal = yLb; yVal <= yUb; ++yVal) {
        solver->setValue(solver->currentTimestamp(), b, bVal);
        solver->setValue(solver->currentTimestamp(), x, xVal);
        solver->setValue(solver->currentTimestamp(), y, yVal);

        const Int expectedOutput = computeOutput(bVal, xVal, yVal);
        invariant.recompute(solver->currentTimestamp());
        EXPECT_EQ(expectedOutput,
                  solver->value(solver->currentTimestamp(), outputId));
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

  solver->open();
  std::array<VarId, 3> inputs{solver->makeIntVar(bLb, bLb, bUb),
                              solver->makeIntVar(ub, lb, ub),
                              solver->makeIntVar(ub, lb, ub)};
  VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  IfThenElse& invariant = solver->makeInvariant<IfThenElse>(
      *solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
  solver->close();

  Timestamp ts = solver->currentTimestamp();

  for (Int bVal = bLb; bVal <= bUb; ++bVal) {
    for (Int val = lb; val <= ub; ++val) {
      for (size_t i = 1; i < inputs.size(); ++i) {
        ++ts;
        solver->setValue(ts, inputs.at(0), bVal);
        solver->setValue(ts, inputs.at(i), val);
        const Int expectedOutput = computeOutput(ts, inputs);

        invariant.notifyInputChanged(ts, LocalId(i));
        EXPECT_EQ(expectedOutput, solver->value(ts, outputId));
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

  solver->open();
  const std::array<VarId, 3> inputs = {solver->makeIntVar(bLb, bLb, bUb),
                                       solver->makeIntVar(lb, lb, ub),
                                       solver->makeIntVar(ub, lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  IfThenElse& invariant = solver->makeInvariant<IfThenElse>(
      *solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    // First input is b,
    // Second input is x if b = 0, otherwise y:
    for (size_t i = 0; i < 2; ++i) {
      const VarId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    const Int bVal = solver->value(ts, inputs.at(0));

    EXPECT_TRUE(notified.at(inputs.at(0)));
    EXPECT_TRUE(notified.at(inputs.at(bVal == 0 ? 1 : 2)));
    EXPECT_FALSE(notified.at(inputs.at(bVal == 0 ? 2 : 1)));
  }
}

TEST_F(IfThenElseTest, NotifyCurrentInputChanged) {
  const Int lb = -5;
  const Int ub = 5;
  const Int bLb = 0;
  const Int bUb = 5;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(bLb <= bUb);

  solver->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<Int> bDist(bLb, bUb);

  const std::array<VarId, 3> inputs = {
      solver->makeIntVar(bLb, bLb, bLb),
      solver->makeIntVar(valueDist(gen), lb, ub),
      solver->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  IfThenElse& invariant = solver->makeInvariant<IfThenElse>(
      *solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
  solver->close();

  for (Timestamp ts = solver->currentTimestamp() + 1;
       ts < solver->currentTimestamp() + 4; ++ts) {
    for (size_t i = 0; i < 2; ++i) {
      const Int bOld = solver->value(ts, inputs.at(0));
      const VarId curInput = invariant.nextInput(ts);
      EXPECT_EQ(curInput, inputs.at(i == 0 ? 0 : bOld == 0 ? 1 : 2));

      const Int oldVal = solver->value(ts, curInput);
      do {
        solver->setValue(ts, curInput, i == 0 ? bDist(gen) : valueDist(gen));
      } while (solver->value(ts, curInput) == oldVal);

      invariant.notifyCurrentInputChanged(ts);

      EXPECT_EQ(solver->value(ts, outputId), computeOutput(ts, inputs));
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

  solver->open();
  std::uniform_int_distribution<Int> bDist(bLb, bUb);
  std::uniform_int_distribution<Int> valueDist(lb, ub);

  std::array<size_t, 3> indices{0, 1, 2};
  std::array<Int, 3> committedValues{bDist(gen), valueDist(gen),
                                     valueDist(gen)};
  std::array<VarId, 3> inputs{
      solver->makeIntVar(committedValues.at(0), bLb, bUb),
      solver->makeIntVar(committedValues.at(1), lb, ub),
      solver->makeIntVar(committedValues.at(2), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = solver->makeIntVar(0, 0, 2);
  IfThenElse& invariant = solver->makeInvariant<IfThenElse>(
      *solver, outputId, inputs.at(0), inputs.at(1), inputs.at(2));
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
      solver->setValue(ts, inputs.at(i), i == 0 ? bDist(gen) : valueDist(gen));
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

class MockIfThenElse : public IfThenElse {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    IfThenElse::registerVars();
  }
  explicit MockIfThenElse(SolverBase& solver, VarId output, VarId b, VarId x,
                          VarId y)
      : IfThenElse(solver, output, b, x, y) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(IfThenElseTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId b = solver->makeIntVar(0, -100, 100);
    const VarId x = solver->makeIntVar(0, 0, 4);
    const VarId y = solver->makeIntVar(5, 5, 9);
    const VarId output = solver->makeIntVar(3, 0, 9);
    testNotifications<MockIfThenElse>(
        &solver->makeInvariant<MockIfThenElse>(*solver, output, b, x, y),
        {propMode, markingMode, 3, b, 5, output});
  }
}

}  // namespace atlantis::testing
