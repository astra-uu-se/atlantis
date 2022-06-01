#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/globalCardinalityConst.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

namespace {

template <bool IsClosed>
class GlobalCardinalityConstTest : public InvariantTest {
 public:
  Int computeViolation(
      const Timestamp ts, const std::vector<VarId>& variables,
      const std::unordered_map<Int, std::pair<Int, Int>>& coverSet) {
    std::vector<Int> values(variables.size(), 0);
    for (size_t i = 0; i < variables.size(); ++i) {
      values.at(i) = engine->value(ts, variables.at(i));
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

    engine->open();
    std::vector<VarId> inputs{engine->makeIntVar(0, 0, 2),
                              engine->makeIntVar(0, 0, 2),
                              engine->makeIntVar(0, 0, 2)};

    for (const auto& [low, up] : lowUpVector) {
      if (!engine->isOpen()) {
        engine->open();
      }
      const VarId violationId = engine->makeIntVar(0, 0, 2);
      GlobalCardinalityConst<IsClosed>& invariant =
          engine->makeConstraint<GlobalCardinalityConst<IsClosed>>(
              violationId, std::vector<VarId>(inputs), std::vector<Int>{1},
              std::vector<Int>{low}, std::vector<Int>{up});
      engine->close();
      EXPECT_EQ(engine->lowerBound(violationId), 0);

      for (Int aVal = lb; aVal <= ub; ++aVal) {
        engine->setValue(engine->currentTimestamp(), inputs.at(0), aVal);
        for (Int bVal = lb; bVal <= ub; ++bVal) {
          engine->setValue(engine->currentTimestamp(), inputs.at(1), bVal);
          for (Int cVal = lb; cVal <= ub; ++cVal) {
            engine->setValue(engine->currentTimestamp(), inputs.at(2), cVal);
            invariant.compute(engine->currentTimestamp(), *engine);
            const Int viol =
                engine->value(engine->currentTimestamp(), violationId);
            EXPECT_TRUE(viol <= engine->upperBound(violationId));
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

      engine->open();
      const VarId a = engine->makeIntVar(lb, lb, ub);
      const VarId b = engine->makeIntVar(lb, lb, ub);
      const VarId c = engine->makeIntVar(lb, lb, ub);
      const VarId violationId = engine->makeIntVar(0, 0, 2);
      GlobalCardinalityConst<IsClosed>& invariant =
          engine->makeConstraint<GlobalCardinalityConst<IsClosed>>(
              violationId, std::vector<VarId>{a, b, c}, cover, low, up);
      engine->close();

      const std::vector<VarId> inputs{a, b, c};

      for (Int aVal = lb; aVal <= ub; ++aVal) {
        for (Int bVal = lb; bVal <= ub; ++bVal) {
          for (Int cVal = lb; cVal <= ub; ++cVal) {
            engine->setValue(engine->currentTimestamp(), a, aVal);
            engine->setValue(engine->currentTimestamp(), b, bVal);
            engine->setValue(engine->currentTimestamp(), c, cVal);
            const Int expectedViolation =
                computeViolation(engine->currentTimestamp(), inputs, coverSet);
            invariant.recompute(engine->currentTimestamp(), *engine);
            const Int actualViolation =
                engine->value(engine->currentTimestamp(), violationId);
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

      engine->open();
      std::vector<VarId> inputs{engine->makeIntVar(lb, lb, ub),
                                engine->makeIntVar(lb, lb, ub),
                                engine->makeIntVar(lb, lb, ub)};
      const VarId violationId = engine->makeIntVar(0, 0, 2);
      GlobalCardinalityConst<IsClosed>& invariant =
          engine->makeConstraint<GlobalCardinalityConst<IsClosed>>(
              violationId, std::vector<VarId>(inputs), cover, low, up);
      engine->close();

      for (Int val = lb; val <= ub; ++val) {
        for (size_t j = 0; j < inputs.size(); ++j) {
          engine->setValue(engine->currentTimestamp(), inputs[j], val);
          const Int expectedViolation =
              computeViolation(engine->currentTimestamp(), inputs, coverSet);

          invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                       LocalId(j));
          EXPECT_EQ(expectedViolation,
                    engine->value(engine->currentTimestamp(), violationId));
        }
      }
    }
  }
  void nextInput() {
    const size_t numInputs = 1000;
    const Int lb = 0;
    const Int ub = numInputs - 1;
    EXPECT_TRUE(lb <= ub);

    engine->open();
    std::vector<size_t> indices;
    std::vector<Int> committedValues;
    std::vector<VarId> inputs;
    std::vector<Int> cover;
    std::vector<Int> low;
    std::vector<Int> up;
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.emplace_back(engine->makeIntVar(i, lb, ub));
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

    const VarId violationId = engine->makeIntVar(0, 0, 2);
    GlobalCardinalityConst<IsClosed>& invariant =
        engine->makeConstraint<GlobalCardinalityConst<IsClosed>>(
            violationId, std::vector<VarId>(inputs), cover, low, up);
    engine->close();

    for (Timestamp ts = engine->currentTimestamp() + 1;
         ts < engine->currentTimestamp() + 4; ++ts) {
      std::vector<bool> notified(maxVarId + 1, false);
      for (size_t i = 0; i < numInputs; ++i) {
        const VarId varId = invariant.nextInput(ts, *engine);
        EXPECT_NE(varId, NULL_ID);
        EXPECT_TRUE(minVarId <= varId);
        EXPECT_TRUE(varId <= maxVarId);
        EXPECT_FALSE(notified.at(varId));
        notified[varId] = true;
      }
      EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
      for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
        EXPECT_TRUE(notified.at(varId));
      }
    }
  }

  void notifyCurrentInputChanged() {
    const Int lb = -10;
    const Int ub = 10;
    EXPECT_TRUE(lb <= ub);

    engine->open();
    const size_t numInputs = 100;
    std::uniform_int_distribution<Int> valueDist(lb, ub);
    std::vector<VarId> inputs;
    std::vector<Int> cover;
    std::vector<Int> low;
    std::vector<Int> up;
    for (size_t i = 0; i < numInputs; ++i) {
      inputs.emplace_back(engine->makeIntVar(valueDist(gen), lb, ub));
      cover.emplace_back(i);
      low.emplace_back(1);
      up.emplace_back(2);
    }

    std::unordered_map<Int, std::pair<Int, Int>> coverSet;
    for (size_t j = 0; j < cover.size(); ++j) {
      coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
    }

    const VarId violationId = engine->makeIntVar(0, 0, numInputs - 1);
    GlobalCardinalityConst<IsClosed>& invariant =
        engine->makeConstraint<GlobalCardinalityConst<IsClosed>>(
            violationId, std::vector<VarId>(inputs), cover, low, up);
    engine->close();

    for (Timestamp ts = engine->currentTimestamp() + 1;
         ts < engine->currentTimestamp() + 4; ++ts) {
      for (const VarId varId : inputs) {
        EXPECT_EQ(invariant.nextInput(ts, *engine), varId);
        const Int oldVal = engine->value(ts, varId);
        do {
          engine->setValue(ts, varId, valueDist(gen));
        } while (engine->value(ts, varId) == oldVal);
        invariant.notifyCurrentInputChanged(ts, *engine);
        EXPECT_EQ(engine->value(ts, violationId),
                  computeViolation(ts, inputs, coverSet));
      }
    }
  }

  void commit() {
    const Int lb = -10;
    const Int ub = 10;
    EXPECT_TRUE(lb <= ub);

    engine->open();
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
      inputs.emplace_back(engine->makeIntVar(committedValues.back(), lb, ub));
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

    const VarId violationId = engine->makeIntVar(0, 0, 2);
    GlobalCardinalityConst<IsClosed>& invariant =
        engine->makeConstraint<GlobalCardinalityConst<IsClosed>>(
            violationId, std::vector<VarId>(inputs), cover, low, up);
    engine->close();

    EXPECT_EQ(engine->value(engine->currentTimestamp(), violationId),
              computeViolation(engine->currentTimestamp(), inputs, coverSet));

    for (const size_t i : indices) {
      Timestamp ts = engine->currentTimestamp() + Timestamp(i);
      for (size_t j = 0; j < numInputs; ++j) {
        // Check that we do not accidentally commit:
        ASSERT_EQ(engine->committedValue(inputs.at(j)), committedValues.at(j));
      }

      const Int oldVal = committedValues.at(i);
      do {
        engine->setValue(ts, inputs.at(i), valueDist(gen));
      } while (oldVal == engine->value(ts, inputs.at(i)));

      // notify changes
      invariant.notifyInputChanged(ts, *engine, LocalId(i));

      // incremental value
      const Int notifiedViolation = engine->value(ts, violationId);
      invariant.recompute(ts, *engine);

      ASSERT_EQ(notifiedViolation, engine->value(ts, violationId));

      engine->commitIf(ts, inputs.at(i));
      committedValues.at(i) = engine->value(ts, inputs.at(i));
      engine->commitIf(ts, violationId);

      invariant.commit(ts, *engine);
      invariant.recompute(ts + 1, *engine);
      ASSERT_EQ(notifiedViolation, engine->value(ts + 1, violationId));
    }
  }

  void rapidCheck(size_t nVar, Int valOffset) {
    size_t numVariables = static_cast<size_t>(nVar) + size_t(2);

    Int valLb = static_cast<Int>(valOffset - numVariables);
    Int valUb = static_cast<Int>(valOffset + numVariables);

    std::random_device rd;
    auto valDistribution = std::uniform_int_distribution<Int>{valLb, valUb};
    auto valGen = std::mt19937(rd());

    std::vector<Int> coverCounts(valUb - valLb + 1, 0);
    std::vector<VarId> variables;

    engine->open();
    for (size_t i = 0; i < numVariables; i++) {
      const Int val = valDistribution(valGen);
      coverCounts[val - valLb] += 1;
      variables.emplace_back(engine->makeIntVar(valLb, valLb, valUb));
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

    VarId viol = engine->makeIntVar(0, 0, static_cast<Int>(numVariables));

    engine->makeInvariant<GlobalCardinalityConst<IsClosed>>(
        viol, variables, cover, lowerBound, upperBound);

    engine->close();

    // There are currently a bug in PropagationEngine that is resolved in
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
        engine->open();
        engine->setPropagationMode(propMode);
        engine->setOutputToInputMarkingMode(markMode);
        engine->close();

        engine->beginMove();
        for (const VarId x : variables) {
          engine->setValue(x, valDistribution(valGen));
        }
        engine->endMove();

        engine->beginProbe();
        engine->query(viol);
        engine->endProbe();

        std::unordered_map<Int, std::pair<Int, Int>> bounds;
        std::unordered_map<Int, Int> actualCounts;
        Int outsideCount = 0;

        for (size_t i = 0; i < cover.size(); ++i) {
          bounds.emplace(cover[i], std::pair(lowerBound[i], upperBound[i]));
          actualCounts.emplace(cover[i], 0);
        }

        for (const VarId varId : variables) {
          Int val = engine->currentValue(varId);
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

        Int actualViolation = engine->currentValue(viol);
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
  void registerVars(Engine& engine) override {
    registered = true;
    GlobalCardinalityConst<IsClosed>::registerVars(engine);
  }
  explicit MockGlobalCardinalityConst(VarId violationId,
                                      const std::vector<VarId>& variables,
                                      const std::vector<Int>& cover,
                                      const std::vector<Int>& counts)
      : GlobalCardinalityConst<IsClosed>(violationId, variables, cover,
                                         counts) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return GlobalCardinalityConst<IsClosed>::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return GlobalCardinalityConst<IsClosed>::nextInput(timestamp, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          GlobalCardinalityConst<IsClosed>::notifyCurrentInputChanged(timestamp,
                                                                      engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault(
            [this](Timestamp timestamp, Engine& engine, LocalId localId) {
              GlobalCardinalityConst<IsClosed>::notifyInputChanged(
                  timestamp, engine, localId);
            });
    ON_CALL(*this, commit)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          GlobalCardinalityConst<IsClosed>::commit(timestamp, engine);
        });
  }
  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));
  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp timestamp, Engine& engine, LocalId localId),
              (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
};
TEST_F(GlobalCardinalityTestClosed, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    Int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(0, -100, 100));
    }

    VarId viol = engine->makeIntVar(0, 0, numArgs);

    testNotifications<MockGlobalCardinalityConst<false>>(
        &engine->makeInvariant<MockGlobalCardinalityConst<false>>(
            viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
            std::vector<Int>{1, 2, 3}),
        propMode, markingMode, numArgs + 1, args.front(), 1, viol);
  }
}
TEST_F(GlobalCardinalityTestOpen, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    Int numArgs = 10;
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(0, -100, 100));
    }

    VarId viol = engine->makeIntVar(0, 0, numArgs);

    testNotifications<MockGlobalCardinalityConst<true>>(
        &engine->makeInvariant<MockGlobalCardinalityConst<true>>(
            viol, std::vector<VarId>{args}, std::vector<Int>{1, 2, 3},
            std::vector<Int>{1, 2, 3}),
        propMode, markingMode, numArgs + 1, args.front(), 1, viol);
  }
}

}  // namespace
