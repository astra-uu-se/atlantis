#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/violationInvariants/globalCardinalityConst.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

template <bool IsClosed>
class GlobalCardinalityConstTest : public InvariantTest {
 public:
  Int computeViolation(
      const Timestamp ts, const std::vector<VarId>& vars,
      const std::unordered_map<Int, std::pair<Int, Int>>& coverSet) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = solver->value(ts, vars.at(i));
    }
    return computeViolation(values, coverSet);
  }

  Int computeViolation(
      const std::vector<Int>& values,
      const std::unordered_map<Int, std::pair<Int, Int>>& coverSet) {
    std::vector<bool> checked(values.size(), false);
    std::unordered_map<Int, Int> actual;
    for (const Int val : values) {
      if (actual.count(val) > 0) {
        ++actual[val];
      } else {
        actual.emplace(val, 1);
      }
    }
    Int shortage = 0;
    Int excess = 0;
    for (const auto& [val, lu] : coverSet) {
      const auto [l, u] = lu;
      shortage +=
          std::max(Int(0), l - (actual.count(val) > 0 ? actual[val] : 0));
      excess += std::max(Int(0), (actual.count(val) > 0 ? actual[val] : 0) - u);
    }
    if constexpr (IsClosed) {
      for (const auto& [val, count] : actual) {
        if (coverSet.count(val) <= 0) {
          excess += count;
        }
      }
    }
    return std::max(shortage, excess);
  }

  void updateBounds() {
    const Int lb = 0;
    const Int ub = 2;
    std::vector<std::pair<Int, Int>> lowUpVector{
        {0, 0}, {0, 4}, {3, 3}, {4, 5}};

    solver->open();
    std::vector<VarId> inputs{solver->makeIntVar(0, 0, 2),
                              solver->makeIntVar(0, 0, 2),
                              solver->makeIntVar(0, 0, 2)};

    for (const auto& [low, up] : lowUpVector) {
      if (!solver->isOpen()) {
        solver->open();
      }
      const VarId violationId = solver->makeIntVar(0, 0, 2);
      GlobalCardinalityConst<IsClosed>& invariant =
          solver->makeViolationInvariant<GlobalCardinalityConst<IsClosed>>(
              *solver, violationId, std::vector<VarId>(inputs),
              std::vector<Int>{1}, std::vector<Int>{low}, std::vector<Int>{up});
      solver->close();
      EXPECT_EQ(solver->lowerBound(violationId), 0);

      for (Int aVal = lb; aVal <= ub; ++aVal) {
        solver->setValue(solver->currentTimestamp(), inputs.at(0), aVal);
        for (Int bVal = lb; bVal <= ub; ++bVal) {
          solver->setValue(solver->currentTimestamp(), inputs.at(1), bVal);
          for (Int cVal = lb; cVal <= ub; ++cVal) {
            solver->setValue(solver->currentTimestamp(), inputs.at(2), cVal);
            invariant.compute(solver->currentTimestamp());
            const Int viol =
                solver->value(solver->currentTimestamp(), violationId);
            EXPECT_TRUE(viol <= solver->upperBound(violationId));
          }
        }
      }
    }
  }

  void recompute() {
    std::vector<std::pair<Int, Int>> boundVec{
        {-10004, -10000}, {-2, 2}, {10000, 10002}};

    std::vector<std::array<std::vector<Int>, 3>> paramVec{
        {std::vector<Int>{-10003, -10002}, std::vector<Int>{0, 1},
         std::vector<Int>{1, 2}},
        {std::vector<Int>{-2, 2}, std::vector<Int>{2, 2},
         std::vector<Int>{3, 5}},
        {std::vector<Int>{10000, 10002}, std::vector<Int>{1, 1},
         std::vector<Int>{2, 2}}};

    for (size_t i = 0; i < boundVec.size(); ++i) {
      auto const [lb, ub] = boundVec[i];
      auto const [cover, low, up] = paramVec[i];
      EXPECT_TRUE(lb <= ub);

      std::unordered_map<Int, std::pair<Int, Int>> coverSet;

      for (size_t j = 0; j < cover.size(); ++j) {
        coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
      }

      solver->open();
      const VarId a = solver->makeIntVar(lb, lb, ub);
      const VarId b = solver->makeIntVar(lb, lb, ub);
      const VarId c = solver->makeIntVar(lb, lb, ub);
      const VarId violationId = solver->makeIntVar(0, 0, 2);
      GlobalCardinalityConst<IsClosed>& invariant =
          solver->makeViolationInvariant<GlobalCardinalityConst<IsClosed>>(
              *solver, violationId, std::vector<VarId>{a, b, c}, cover, low,
              up);
      solver->close();

      const std::vector<VarId> inputs{a, b, c};

      for (Int aVal = lb; aVal <= ub; ++aVal) {
        for (Int bVal = lb; bVal <= ub; ++bVal) {
          for (Int cVal = lb; cVal <= ub; ++cVal) {
            solver->setValue(solver->currentTimestamp(), a, aVal);
            solver->setValue(solver->currentTimestamp(), b, bVal);
            solver->setValue(solver->currentTimestamp(), c, cVal);
            const Int expectedViolation =
                computeViolation(solver->currentTimestamp(), inputs, coverSet);
            invariant.recompute(solver->currentTimestamp());
            const Int actualViolation =
                solver->value(solver->currentTimestamp(), violationId);
            if (expectedViolation != actualViolation) {
              EXPECT_EQ(expectedViolation, actualViolation);
            }
          }
        }
      }
    }
  }

  void notifyInputChanged() {
    std::vector<std::pair<Int, Int>> boundVec{
        {-10002, -10000}, {-1, 1}, {10000, 10002}};

    std::vector<std::array<std::vector<Int>, 3>> paramVec{
        {std::vector<Int>{-10003, -10002}, std::vector<Int>{0, 1},
         std::vector<Int>{1, 2}},
        {std::vector<Int>{-2, 2}, std::vector<Int>{2, 2},
         std::vector<Int>{3, 5}},
        {std::vector<Int>{10000, 10002}, std::vector<Int>{1, 1},
         std::vector<Int>{2, 2}}};

    for (size_t i = 0; i < boundVec.size(); ++i) {
      auto const [lb, ub] = boundVec[i];
      auto const [cover, low, up] = paramVec[i];
      EXPECT_TRUE(lb <= ub);

      std::unordered_map<Int, std::pair<Int, Int>> coverSet;
      for (size_t j = 0; j < cover.size(); ++j) {
        coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
      }

      solver->open();
      std::vector<VarId> inputs{solver->makeIntVar(lb, lb, ub),
                                solver->makeIntVar(lb, lb, ub),
                                solver->makeIntVar(lb, lb, ub)};
      const VarId violationId = solver->makeIntVar(0, 0, 2);
      GlobalCardinalityConst<IsClosed>& invariant =
          solver->makeViolationInvariant<GlobalCardinalityConst<IsClosed>>(
              *solver, violationId, std::vector<VarId>(inputs), cover, low, up);
      solver->close();

      Timestamp ts = solver->currentTimestamp();

      for (Int val = lb; val <= ub; ++val) {
        ++ts;
        for (size_t j = 0; j < inputs.size(); ++j) {
          solver->setValue(ts, inputs[j], val);
          const Int expectedViolation = computeViolation(ts, inputs, coverSet);

          invariant.notifyInputChanged(ts, LocalId(j));
          EXPECT_EQ(expectedViolation, solver->value(ts, violationId));
        }
      }
    }
  }
  void nextInput() {
    const Int numInputs = 1000;
    const Int lb = 0;
    const Int ub = numInputs - 1;
    EXPECT_TRUE(lb <= ub);

    solver->open();
    std::vector<size_t> indices;
    std::vector<Int> committedValues;
    std::vector<VarId> inputs;
    std::vector<Int> cover;
    std::vector<Int> low;
    std::vector<Int> up;
    for (Int i = 0; i < numInputs; ++i) {
      inputs.emplace_back(solver->makeIntVar(i, lb, ub));
      cover.emplace_back(i);
      low.emplace_back(1);
      up.emplace_back(2);
    }

    std::unordered_map<Int, std::pair<Int, Int>> coverSet;
    for (size_t j = 0; j < cover.size(); ++j) {
      coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
    }

    const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
    const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

    std::shuffle(inputs.begin(), inputs.end(), rng);

    const VarId violationId = solver->makeIntVar(0, 0, 2);
    GlobalCardinalityConst<IsClosed>& invariant =
        solver->makeViolationInvariant<GlobalCardinalityConst<IsClosed>>(
            *solver, violationId, std::vector<VarId>(inputs), cover, low, up);
    solver->close();

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

  void notifyCurrentInputChanged() {
    const Int lb = -10;
    const Int ub = 10;
    EXPECT_TRUE(lb <= ub);

    solver->open();
    const size_t numInputs = 100;
    std::uniform_int_distribution<Int> valueDist(lb, ub);
    std::vector<VarId> inputs;
    std::vector<Int> cover;
    std::vector<Int> low;
    std::vector<Int> up;
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.emplace_back(solver->makeIntVar(valueDist(gen), lb, ub));
      cover.emplace_back(i);
      low.emplace_back(1);
      up.emplace_back(2);
    }

    std::unordered_map<Int, std::pair<Int, Int>> coverSet;
    for (size_t j = 0; j < cover.size(); ++j) {
      coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
    }

    const VarId violationId = solver->makeIntVar(0, 0, numInputs - 1);
    GlobalCardinalityConst<IsClosed>& invariant =
        solver->makeViolationInvariant<GlobalCardinalityConst<IsClosed>>(
            *solver, violationId, std::vector<VarId>(inputs), cover, low, up);
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
        EXPECT_EQ(solver->value(ts, violationId),
                  computeViolation(ts, inputs, coverSet));
      }
    }
  }

  void commit() {
    const Int lb = -10;
    const Int ub = 10;
    EXPECT_TRUE(lb <= ub);

    solver->open();
    const size_t numInputs = 1000;
    std::uniform_int_distribution<Int> valueDist(lb, ub);
    std::uniform_int_distribution<size_t> varDist(size_t(0), numInputs);
    std::vector<size_t> indices;
    std::vector<Int> committedValues;
    std::vector<VarId> inputs;
    std::vector<Int> cover;
    std::vector<Int> low;
    std::vector<Int> up;
    for (size_t i = 0; i < numInputs; ++i) {
      indices.emplace_back(i);
      committedValues.emplace_back(valueDist(gen));
      inputs.emplace_back(solver->makeIntVar(committedValues.back(), lb, ub));
      const Int b1 = varDist(gen);
      const Int b2 = varDist(gen);
      cover.emplace_back(i);
      low.emplace_back(std::min(b1, b2));
      up.emplace_back(std::max(b1, b2));
    }
    std::unordered_map<Int, std::pair<Int, Int>> coverSet;
    for (size_t j = 0; j < cover.size(); ++j) {
      coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
    }
    std::shuffle(indices.begin(), indices.end(), rng);

    const VarId violationId = solver->makeIntVar(0, 0, 2);
    GlobalCardinalityConst<IsClosed>& invariant =
        solver->makeViolationInvariant<GlobalCardinalityConst<IsClosed>>(
            *solver, violationId, std::vector<VarId>(inputs), cover, low, up);
    solver->close();

    EXPECT_EQ(solver->value(solver->currentTimestamp(), violationId),
              computeViolation(solver->currentTimestamp(), inputs, coverSet));

    for (const size_t i : indices) {
      Timestamp ts = solver->currentTimestamp() + Timestamp(i);
      for (size_t j = 0; j < numInputs; ++j) {
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
      const Int notifiedViolation = solver->value(ts, violationId);
      invariant.recompute(ts);

      ASSERT_EQ(notifiedViolation, solver->value(ts, violationId));

      solver->commitIf(ts, inputs.at(i));
      committedValues.at(i) = solver->value(ts, inputs.at(i));
      solver->commitIf(ts, violationId);

      invariant.commit(ts);
      invariant.recompute(ts + 1);
      ASSERT_EQ(notifiedViolation, solver->value(ts + 1, violationId));
    }
  }

  void rapidCheck(size_t nVar, Int valOffset) {
    size_t numVars = static_cast<size_t>(nVar) + size_t(2);

    Int valLb = static_cast<Int>(valOffset - numVars);
    Int valUb = static_cast<Int>(valOffset + numVars);

    std::random_device rd;
    auto valDistribution = std::uniform_int_distribution<Int>{valLb, valUb};
    auto valGen = std::mt19937(rd());

    std::vector<Int> coverCounts(valUb - valLb + 1, 0);
    std::vector<VarId> vars;

    solver->open();
    for (size_t i = 0; i < numVars; i++) {
      const Int val = valDistribution(valGen);
      coverCounts[val - valLb] += 1;
      vars.emplace_back(solver->makeIntVar(valLb, valLb, valUb));
    }

    std::vector<Int> cover;
    std::vector<Int> lowerBound;
    std::vector<Int> upperBound;
    for (size_t i = 0; i < coverCounts.size(); ++i) {
      if (coverCounts[i] > 0) {
        cover.emplace_back(i + valLb);
        lowerBound.emplace_back(std::max(Int(0), coverCounts[i] - 1));
        upperBound.emplace_back(std::max(Int(0), coverCounts[i]));
      }
    }

    VarId viol = solver->makeIntVar(0, 0, static_cast<Int>(numVars));

    solver->makeInvariant<GlobalCardinalityConst<IsClosed>>(
        *solver, viol, std::vector<VarId>(vars), cover, lowerBound, upperBound);

    solver->close();

    // There are currently a bug in Solver that is resolved in
    // another branch.
    for (auto [propMode, markMode] :
         std::vector<std::pair<PropagationMode, OutputToInputMarkingMode>>{
             {PropagationMode::INPUT_TO_OUTPUT,
              OutputToInputMarkingMode::NONE}  //,
             //{PropagationMode::OUTPUT_TO_INPUT,
             // OutputToInputMarkingMode::NONE},
             //{PropagationMode::OUTPUT_TO_INPUT,
             // OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION},
             //{PropagationMode::OUTPUT_TO_INPUT,
             // OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC}
         }) {
      for (size_t iter = 0; iter < 3; ++iter) {
        solver->open();
        solver->setPropagationMode(propMode);
        solver->setOutputToInputMarkingMode(markMode);
        solver->close();

        solver->beginMove();
        for (const VarId x : vars) {
          solver->setValue(x, valDistribution(valGen));
        }
        solver->endMove();

        solver->beginProbe();
        solver->query(viol);
        solver->endProbe();

        std::unordered_map<Int, std::pair<Int, Int>> bounds;
        std::unordered_map<Int, Int> actualCounts;
        Int outsideCount = 0;

        for (size_t i = 0; i < cover.size(); ++i) {
          bounds.emplace(cover[i], std::pair(lowerBound[i], upperBound[i]));
          actualCounts.emplace(cover[i], 0);
        }

        for (const VarId varId : vars) {
          Int val = solver->currentValue(varId);
          if (actualCounts.count(val) <= 0) {
            ++outsideCount;
          } else {
            ++actualCounts[val];
          }
        }

        Int shortage = 0;
        Int excess = 0;
        if constexpr (IsClosed) {
          excess = outsideCount;
        }

        for (const Int val : cover) {
          RC_ASSERT(actualCounts.count(val) > size_t(0) &&
                    bounds.count(val) > size_t(0));
          const auto [l, u] = bounds.at(val);
          const auto actual = actualCounts.at(val);
          shortage += std::max(Int(0), l - actual);
          excess += std::max(Int(0), actual - u);
        }

        Int actualViolation = solver->currentValue(viol);
        Int expectedViolation = std::max(shortage, excess);
        if (actualViolation != expectedViolation) {
          RC_ASSERT(actualViolation == expectedViolation);
        }
      }
    }
  }
};

class GlobalCardinalityTestClosed : public GlobalCardinalityConstTest<true> {};
class GlobalCardinalityTestOpen : public GlobalCardinalityConstTest<false> {};

TEST_F(GlobalCardinalityTestClosed, UpdateBounds) { updateBounds(); }

TEST_F(GlobalCardinalityTestOpen, UpdateBounds) { updateBounds(); }

TEST_F(GlobalCardinalityTestClosed, Recompute) { recompute(); }

TEST_F(GlobalCardinalityTestOpen, Recompute) { recompute(); }

TEST_F(GlobalCardinalityTestClosed, NotifyInputChanged) {
  notifyInputChanged();
}

TEST_F(GlobalCardinalityTestOpen, NotifyInputChanged) { notifyInputChanged(); }

TEST_F(GlobalCardinalityTestClosed, NextInput) { nextInput(); }

TEST_F(GlobalCardinalityTestOpen, NextInput) { nextInput(); }

TEST_F(GlobalCardinalityTestClosed, NotifyCurrentInputChanged) {
  notifyCurrentInputChanged();
}

TEST_F(GlobalCardinalityTestOpen, NotifyCurrentInputChanged) {
  notifyCurrentInputChanged();
}

TEST_F(GlobalCardinalityTestClosed, Commit) { commit(); }

TEST_F(GlobalCardinalityTestOpen, Commit) { commit(); }

RC_GTEST_FIXTURE_PROP(GlobalCardinalityTestOpen, RapidCheck,
                      (unsigned char nVar, int valOffset)) {
  rapidCheck(static_cast<size_t>(nVar), static_cast<Int>(valOffset));
}

RC_GTEST_FIXTURE_PROP(GlobalCardinalityTestClosed, RapidCheck,
                      (unsigned char nVar, int valOffset)) {
  rapidCheck(static_cast<size_t>(nVar), static_cast<Int>(valOffset));
}

template <bool IsClosed>
class MockGlobalCardinalityConst : public GlobalCardinalityConst<IsClosed> {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    GlobalCardinalityConst<IsClosed>::registerVars();
  }
  explicit MockGlobalCardinalityConst(SolverBase& solver, VarId violationId,
                                      std::vector<VarId>&& vars,
                                      const std::vector<Int>& cover,
                                      const std::vector<Int>& counts)
      : GlobalCardinalityConst<IsClosed>(solver, violationId, std::move(vars), cover,
                                         counts) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return GlobalCardinalityConst<IsClosed>::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return GlobalCardinalityConst<IsClosed>::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          GlobalCardinalityConst<IsClosed>::notifyCurrentInputChanged(
              timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId localId) {
          GlobalCardinalityConst<IsClosed>::notifyInputChanged(timestamp,
                                                               localId);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      GlobalCardinalityConst<IsClosed>::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(GlobalCardinalityTestClosed, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    std::vector<VarId> args;
    size_t numArgs = 10;
    for (size_t value = 0; value < numArgs; ++value) {
      args.push_back(solver->makeIntVar(0, -100, 100));
    }

    VarId viol = solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    VarId modifiedVarId = args.front();
    testNotifications<MockGlobalCardinalityConst<false>>(
        &solver->makeInvariant<MockGlobalCardinalityConst<false>>(
            *solver, viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
            std::vector<Int>{1, 2, 3}),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 1, viol});
  }
}
TEST_F(GlobalCardinalityTestOpen, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    std::vector<VarId> args;
    size_t numArgs = 10;
    for (size_t value = 0; value < numArgs; ++value) {
      args.push_back(solver->makeIntVar(0, -100, 100));
    }

    VarId viol = solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    const VarId modifiedVarId = args.front();

    testNotifications<MockGlobalCardinalityConst<true>>(
        &solver->makeInvariant<MockGlobalCardinalityConst<true>>(
            *solver, viol, std::move(args), std::vector<Int>{1, 2, 3},
            std::vector<Int>{1, 2, 3}),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 1, viol});
  }
}

}  // namespace atlantis::testing
