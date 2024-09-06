#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class GlobalCardinalityLowUpTest : public InvariantTest {
 public:
  Int computeViolation(
      const Timestamp ts, const std::vector<VarViewId>& vars,
      const std::unordered_map<Int, std::pair<Int, Int>>& coverSet) {
    std::vector<Int> values(vars.size(), 0);
    for (size_t i = 0; i < vars.size(); ++i) {
      values.at(i) = _solver->value(ts, vars.at(i));
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
    return std::max(shortage, excess);
  }
};

TEST_F(GlobalCardinalityLowUpTest, UpdateBounds) {
  const Int lb = 0;
  const Int ub = 2;
  std::vector<std::pair<Int, Int>> lowUpVector{{0, 0}, {0, 4}, {3, 3}, {4, 5}};

  _solver->open();
  std::vector<VarViewId> inputs{_solver->makeIntVar(0, 0, 2),
                                _solver->makeIntVar(0, 0, 2),
                                _solver->makeIntVar(0, 0, 2)};

  for (const auto& [low, up] : lowUpVector) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
    GlobalCardinalityLowUp& invariant =
        _solver->makeViolationInvariant<GlobalCardinalityLowUp>(
            *_solver, violationId, std::vector<VarViewId>(inputs),
            std::vector<Int>{1}, std::vector<Int>{low}, std::vector<Int>{up});
    _solver->close();
    EXPECT_EQ(_solver->lowerBound(violationId), 0);

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      _solver->setValue(_solver->currentTimestamp(), inputs.at(0), aVal);
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        _solver->setValue(_solver->currentTimestamp(), inputs.at(1), bVal);
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          _solver->setValue(_solver->currentTimestamp(), inputs.at(2), cVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
          const Int viol =
              _solver->value(_solver->currentTimestamp(), violationId);
          EXPECT_TRUE(viol <= _solver->upperBound(violationId));
        }
      }
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, Recompute) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-10004, -10000}, {-2, 2}, {10000, 10002}};

  std::vector<std::array<std::vector<Int>, 3>> paramVec{
      {std::vector<Int>{-10003, -10002}, std::vector<Int>{0, 1},
       std::vector<Int>{1, 2}},
      {std::vector<Int>{-2, 2}, std::vector<Int>{2, 2}, std::vector<Int>{3, 5}},
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

    _solver->open();
    const VarViewId a = _solver->makeIntVar(lb, lb, ub);
    const VarViewId b = _solver->makeIntVar(lb, lb, ub);
    const VarViewId c = _solver->makeIntVar(lb, lb, ub);
    const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
    GlobalCardinalityLowUp& invariant =
        _solver->makeViolationInvariant<GlobalCardinalityLowUp>(
            *_solver, violationId, std::vector<VarViewId>{a, b, c}, cover, low,
            up);
    _solver->close();

    const std::vector<VarViewId> inputs{a, b, c};

    for (Int aVal = lb; aVal <= ub; ++aVal) {
      for (Int bVal = lb; bVal <= ub; ++bVal) {
        for (Int cVal = lb; cVal <= ub; ++cVal) {
          _solver->setValue(_solver->currentTimestamp(), a, aVal);
          _solver->setValue(_solver->currentTimestamp(), b, bVal);
          _solver->setValue(_solver->currentTimestamp(), c, cVal);
          const Int expectedViolation =
              computeViolation(_solver->currentTimestamp(), inputs, coverSet);
          invariant.recompute(_solver->currentTimestamp());
          const Int actualViolation =
              _solver->value(_solver->currentTimestamp(), violationId);
          if (expectedViolation != actualViolation) {
            EXPECT_EQ(expectedViolation, actualViolation);
          }
        }
      }
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, NotifyInputChanged) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  std::vector<std::array<std::vector<Int>, 3>> paramVec{
      {std::vector<Int>{-10003, -10002}, std::vector<Int>{0, 1},
       std::vector<Int>{1, 2}},
      {std::vector<Int>{-2, 2}, std::vector<Int>{2, 2}, std::vector<Int>{3, 5}},
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

    _solver->open();
    std::vector<VarViewId> inputs{_solver->makeIntVar(lb, lb, ub),
                                  _solver->makeIntVar(lb, lb, ub),
                                  _solver->makeIntVar(lb, lb, ub)};
    const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
    GlobalCardinalityLowUp& invariant =
        _solver->makeViolationInvariant<GlobalCardinalityLowUp>(
            *_solver, violationId, std::vector<VarViewId>(inputs), cover, low,
            up);
    _solver->close();

    Timestamp ts = _solver->currentTimestamp();

    for (Int val = lb; val <= ub; ++val) {
      ++ts;
      for (LocalId j = 0; j < inputs.size(); ++j) {
        _solver->setValue(ts, inputs[j], val);
        const Int expectedViolation = computeViolation(ts, inputs, coverSet);

        invariant.notifyInputChanged(ts, j);
        EXPECT_EQ(expectedViolation, _solver->value(ts, violationId));
      }
    }
  }
}
TEST_F(GlobalCardinalityLowUpTest, NextInput) {
  const Int numInputs = 1000;
  const Int lb = 0;
  const Int ub = numInputs - 1;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarViewId> inputs;
  std::vector<Int> cover;
  std::vector<Int> low;
  std::vector<Int> up;
  for (Int i = 0; i < numInputs; ++i) {
    inputs.emplace_back(_solver->makeIntVar(i, lb, ub));
    cover.emplace_back(i);
    low.emplace_back(1);
    up.emplace_back(2);
  }

  std::unordered_map<Int, std::pair<Int, Int>> coverSet;
  for (size_t j = 0; j < cover.size(); ++j) {
    coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
  }

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

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  GlobalCardinalityLowUp& invariant =
      _solver->makeViolationInvariant<GlobalCardinalityLowUp>(
          *_solver, violationId, std::vector<VarViewId>(inputs), cover, low,
          up);
  _solver->close();

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

TEST_F(GlobalCardinalityLowUpTest, NotifyCurrentInputChanged) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const size_t numInputs = 100;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::vector<VarViewId> inputs;
  std::vector<Int> cover;
  std::vector<Int> low;
  std::vector<Int> up;
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.emplace_back(_solver->makeIntVar(valueDist(gen), lb, ub));
    cover.emplace_back(i);
    low.emplace_back(1);
    up.emplace_back(2);
  }

  std::unordered_map<Int, std::pair<Int, Int>> coverSet;
  for (size_t j = 0; j < cover.size(); ++j) {
    coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
  }

  const VarViewId violationId = _solver->makeIntVar(0, 0, numInputs - 1);
  GlobalCardinalityLowUp& invariant =
      _solver->makeViolationInvariant<GlobalCardinalityLowUp>(
          *_solver, violationId, std::vector<VarViewId>(inputs), cover, low,
          up);
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, valueDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, violationId),
                computeViolation(ts, inputs, coverSet));
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  _solver->open();
  const size_t numInputs = 1000;
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::uniform_int_distribution<size_t> varDist(size_t(0), numInputs);
  std::vector<size_t> indices;
  std::vector<Int> committedValues;
  std::vector<VarViewId> inputs;
  std::vector<Int> cover;
  std::vector<Int> low;
  std::vector<Int> up;
  for (size_t i = 0; i < numInputs; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(valueDist(gen));
    inputs.emplace_back(_solver->makeIntVar(committedValues.back(), lb, ub));
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

  const VarViewId violationId = _solver->makeIntVar(0, 0, 2);
  GlobalCardinalityLowUp& invariant =
      _solver->makeViolationInvariant<GlobalCardinalityLowUp>(
          *_solver, violationId, std::vector<VarViewId>(inputs), cover, low,
          up);
  _solver->close();

  EXPECT_EQ(_solver->value(_solver->currentTimestamp(), violationId),
            computeViolation(_solver->currentTimestamp(), inputs, coverSet));

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == _solver->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId{i});

    // incremental value
    const Int notifiedViolation = _solver->value(ts, violationId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, _solver->value(ts, violationId));

    _solver->commitIf(ts, VarId(inputs.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputs.at(i)));
    _solver->commitIf(ts, VarId(violationId));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, _solver->value(ts + 1, violationId));
  }
}

RC_GTEST_FIXTURE_PROP(GlobalCardinalityLowUpTest, RapidCheck,
                      (unsigned char nVar, int valOffset)) {
  size_t numVars = static_cast<size_t>(nVar) + size_t(2);

  Int valLb = static_cast<Int>(valOffset - numVars);
  Int valUb = static_cast<Int>(valOffset + numVars);

  std::random_device rd;
  auto valDistribution = std::uniform_int_distribution<Int>{valLb, valUb};
  auto valGen = std::mt19937(rd());

  std::vector<Int> coverCounts(valUb - valLb + 1, 0);
  std::vector<VarViewId> vars;

  _solver->open();
  for (size_t i = 0; i < numVars; i++) {
    const Int val = valDistribution(valGen);
    coverCounts[val - valLb] += 1;
    vars.emplace_back(_solver->makeIntVar(valLb, valLb, valUb));
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

  VarViewId viol = _solver->makeIntVar(0, 0, static_cast<Int>(numVars));

  _solver->makeInvariant<GlobalCardinalityLowUp>(*_solver, viol,
                                                 std::vector<VarViewId>(vars),
                                                 cover, lowerBound, upperBound);

  _solver->close();

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
      _solver->open();
      _solver->setPropagationMode(propMode);
      _solver->setOutputToInputMarkingMode(markMode);
      _solver->close();

      _solver->beginMove();
      for (const VarViewId& x : vars) {
        _solver->setValue(x, valDistribution(valGen));
      }
      _solver->endMove();

      _solver->beginProbe();
      _solver->query(viol);
      _solver->endProbe();

      std::unordered_map<Int, std::pair<Int, Int>> bounds;
      std::unordered_map<Int, Int> actualCounts;
      Int outsideCount = 0;

      for (size_t i = 0; i < cover.size(); ++i) {
        bounds.emplace(cover[i], std::pair(lowerBound[i], upperBound[i]));
        actualCounts.emplace(cover[i], 0);
      }

      for (const VarViewId& varId : vars) {
        Int val = _solver->currentValue(varId);
        if (actualCounts.count(val) <= 0) {
          ++outsideCount;
        } else {
          ++actualCounts[val];
        }
      }

      Int shortage = 0;
      Int excess = 0;

      for (const Int val : cover) {
        RC_ASSERT(actualCounts.count(val) > size_t(0) &&
                  bounds.count(val) > size_t(0));
        const auto [l, u] = bounds.at(val);
        const auto actual = actualCounts.at(val);
        shortage += std::max(Int(0), l - actual);
        excess += std::max(Int(0), actual - u);
      }

      Int actualViolation = _solver->currentValue(viol);
      Int expectedViolation = std::max(shortage, excess);
      if (actualViolation != expectedViolation) {
        RC_ASSERT(actualViolation == expectedViolation);
      }
    }
  }
}

class MockGlobalCardinalityConst : public GlobalCardinalityLowUp {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    GlobalCardinalityLowUp::registerVars();
  }
  explicit MockGlobalCardinalityConst(SolverBase& solver, VarViewId violationId,
                                      std::vector<VarViewId>&& vars,
                                      const std::vector<Int>& cover,
                                      const std::vector<Int>& counts)
      : GlobalCardinalityLowUp(solver, violationId, std::move(vars), cover,
                               counts) {
    EXPECT_TRUE(violationId.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return GlobalCardinalityLowUp::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return GlobalCardinalityLowUp::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          GlobalCardinalityLowUp::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId localId) {
          GlobalCardinalityLowUp::notifyInputChanged(timestamp, localId);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      GlobalCardinalityLowUp::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(GlobalCardinalityLowUpTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<VarViewId> args;
    size_t numArgs = 10;
    for (size_t value = 0; value < numArgs; ++value) {
      args.push_back(_solver->makeIntVar(0, -100, 100));
    }

    VarViewId viol = _solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    const VarViewId modifiedVarId = args.front();

    testNotifications<MockGlobalCardinalityConst>(
        &_solver->makeInvariant<MockGlobalCardinalityConst>(
            *_solver, viol, std::move(args), std::vector<Int>{1, 2, 3},
            std::vector<Int>{1, 2, 3}),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 1, viol});
  }
}

}  // namespace atlantis::testing
