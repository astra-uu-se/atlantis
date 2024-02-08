#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/intDiv.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IntDivTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId nominator,
                    const VarId denominator) {
    Int denVal = solver->value(ts, denominator);
    if (denVal == 0) {
      denVal = solver->upperBound(denominator) > 0 ? 1 : -1;
    }
    return solver->value(ts, nominator) / denVal;
  }
};

TEST_F(IntDivTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  solver->open();
  const VarId nominator = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId denominator = solver->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = solver->makeIntVar(0, 0, 2);
  IntDiv& invariant =
      solver->makeInvariant<IntDiv>(*solver, outputId, nominator, denominator);
  solver->close();

  for (const auto& [nomLb, nomUb] : boundVec) {
    EXPECT_TRUE(nomLb <= nomUb);
    solver->updateBounds(nominator, nomLb, nomUb, false);
    for (const auto& [denLb, denUb] : boundVec) {
      EXPECT_TRUE(denLb <= denUb);
      solver->updateBounds(denominator, denLb, denUb, false);
      solver->open();
      invariant.updateBounds(false);
      solver->close();
      std::vector<Int> outputs;
      const Int outLb = solver->lowerBound(outputId);
      const Int outUb = solver->upperBound(outputId);
      for (Int nomVal = nomLb; nomVal <= nomUb; ++nomVal) {
        solver->setValue(solver->currentTimestamp(), nominator, nomVal);
        for (Int denVal = denLb; denVal <= denUb; ++denVal) {
          solver->setValue(solver->currentTimestamp(), denominator, denVal);
          invariant.recompute(solver->currentTimestamp());
          const Int outVal =
              solver->value(solver->currentTimestamp(), outputId);
          if (outVal < outLb || outUb < outVal) {
            ASSERT_TRUE(outLb <= outVal);
            ASSERT_TRUE(outVal <= outUb);
          }
          outputs.emplace_back(outVal);
        }
      }
      const auto& [minVal, maxVal] =
          std::minmax_element(outputs.begin(), outputs.end());
      if (*minVal != solver->lowerBound(outputId)) {
        ASSERT_EQ(*minVal, solver->lowerBound(outputId));
      }
      ASSERT_EQ(*maxVal, solver->upperBound(outputId));
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

  solver->open();
  const VarId nominator = solver->makeIntVar(nomUb, nomLb, nomUb);
  const VarId denominator = solver->makeIntVar(denUb, denLb, denUb);
  const VarId outputId = solver->makeIntVar(0, outputLb, outputUb);
  IntDiv& invariant =
      solver->makeInvariant<IntDiv>(*solver, outputId, nominator, denominator);
  solver->close();

  for (Int nomVal = nomLb; nomVal <= nomUb; ++nomVal) {
    for (Int denVal = denLb; denVal <= denUb; ++denVal) {
      solver->setValue(solver->currentTimestamp(), nominator, nomVal);
      solver->setValue(solver->currentTimestamp(), denominator, denVal);

      const Int expectedOutput =
          computeOutput(solver->currentTimestamp(), nominator, denominator);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(IntDivTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  solver->open();
  std::array<VarId, 2> inputs{solver->makeIntVar(ub, lb, ub),
                              solver->makeIntVar(ub, lb, ub)};
  VarId outputId = solver->makeIntVar(0, 0, ub - lb);
  IntDiv& invariant = solver->makeInvariant<IntDiv>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, NextInput) {
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
  IntDiv& invariant = solver->makeInvariant<IntDiv>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, NotifyCurrentInputChanged) {
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
  IntDiv& invariant = solver->makeInvariant<IntDiv>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, Commit) {
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
  IntDiv& invariant = solver->makeInvariant<IntDiv>(*solver, outputId,
                                                    inputs.at(0), inputs.at(1));
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

TEST_F(IntDivTest, ZeroDenominator) {
  const Int nomVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [denLb, denUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, -10}, {-50, 50, 10}, {0, 100, 10}}) {
    EXPECT_TRUE(denLb <= denUb);
    EXPECT_TRUE(denLb != 0 || denUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      solver->open();
      const VarId nominator = solver->makeIntVar(nomVal, nomVal, nomVal);
      const VarId denominator = solver->makeIntVar(0, denLb, denUb);
      const VarId outputId = solver->makeIntVar(0, outputLb, outputUb);
      IntDiv& invariant = solver->makeInvariant<IntDiv>(*solver, outputId,
                                                        nominator, denominator);
      solver->close();

      EXPECT_EQ(expected, computeOutput(solver->currentTimestamp(), nominator,
                                        denominator));
      if (method == 0) {
        invariant.recompute(solver->currentTimestamp());
      } else {
        invariant.notifyInputChanged(solver->currentTimestamp(), LocalId(1));
      }
      EXPECT_EQ(expected, solver->value(solver->currentTimestamp(), outputId));
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
  explicit MockIntDiv(SolverBase& solver, VarId output, VarId nominator,
                      VarId denominator)
      : IntDiv(solver, output, nominator, denominator) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(IntDivTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    const VarId nominator = solver->makeIntVar(-10, -100, 100);
    const VarId denominator = solver->makeIntVar(10, -100, 100);
    const VarId output = solver->makeIntVar(0, 0, 200);
    testNotifications<MockIntDiv>(
        &solver->makeInvariant<MockIntDiv>(*solver, output, nominator,
                                           denominator),
        {propMode, markingMode, 3, nominator, 0, output});
  }
}

}  // namespace atlantis::testing
