#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <iostream>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class GlobalCardinalityLowUpTest : public InvariantTest {
 public:
  Int numInputVars{3};

  Int inputVarLb{-2};
  Int inputVarUb{2};

  std::vector<VarViewId> inputVars;
  std::uniform_int_distribution<Int> inputVarDist;

  std::unordered_map<Int, std::pair<Int, Int>> coverSet;

  VarViewId outputVar{NULL_ID};

  Int computeOutput(bool committedValue = false) {
    std::vector<Int> values(inputVars.size(), 0);
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = committedValue ? _solver->committedValue(inputVars.at(i))
                                    : _solver->currentValue(inputVars.at(i));
    }
    return computeOutput(values);
  }

  Int computeOutput(Timestamp ts) {
    std::vector<Int> values(inputVars.size(), 0);
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = _solver->value(ts, inputVars.at(i));
    }
    return computeOutput(values);
  }

  Int computeOutput(const std::vector<Int>& values) {
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

  void SetUp() override {
    InvariantTest::SetUp();
    inputVarLb = 0;
    inputVarUb = 2;
    coverSet = std::unordered_map<Int, std::pair<Int, Int>>{
        {0, std::pair<Int, Int>{0, 1}},  // number of 1 at most 1
        {2, std::pair<Int, Int>{1, 2}}   // number of 2 in {1, 2}
    };
  }

  GlobalCardinalityLowUp& generate() {
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);
    inputVars.clear();
    inputVars.reserve(numInputVars);

    if (!_solver->isOpen()) {
      _solver->open();
    }

    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(
          _solver->makeIntVar(inputVarDist(gen), inputVarLb, inputVarUb));
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    std::vector<Int> cover;
    std::vector<Int> low;
    std::vector<Int> up;
    cover.reserve(coverSet.size());
    low.reserve(coverSet.size());
    up.reserve(coverSet.size());

    for (const auto& [val, lu] : coverSet) {
      cover.emplace_back(val);
      low.emplace_back(lu.first);
      up.emplace_back(lu.second);
    }

    GlobalCardinalityLowUp& invariant =
        _solver->makeInvariant<GlobalCardinalityLowUp>(
            *_solver, outputVar, std::vector<VarViewId>(inputVars),
            std::move(cover), std::move(low), std::move(up));
    _solver->close();
    return invariant;
  }
};

TEST_F(GlobalCardinalityLowUpTest, UpdateBounds) {
  inputVarLb = 0;
  inputVarUb = 2;
  std::vector<std::pair<Int, Int>> lowUpVector{{0, 0}, {0, 4}, {3, 3}, {4, 5}};

  for (const auto& p : lowUpVector) {
    coverSet.clear();
    coverSet.emplace(1, p);

    auto& invariant = generate();

    EXPECT_EQ(_solver->lowerBound(outputVar), 0);

    for (Int aVal = inputVarLb; aVal <= inputVarUb; ++aVal) {
      _solver->setValue(_solver->currentTimestamp(), inputVars.at(0), aVal);
      for (Int bVal = inputVarLb; bVal <= inputVarUb; ++bVal) {
        _solver->setValue(_solver->currentTimestamp(), inputVars.at(1), bVal);
        for (Int cVal = inputVarLb; cVal <= inputVarUb; ++cVal) {
          _solver->setValue(_solver->currentTimestamp(), inputVars.at(2), cVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
          const Int viol = _solver->currentValue(outputVar);
          EXPECT_LE(viol, _solver->upperBound(outputVar));
        }
      }
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, Recompute) {
  generateState = GenerateState::LB;

  numInputVars = 3;

  std::vector<std::pair<Int, Int>> boundVec{
      {-10004, -10000}, {-2, 2}, {10000, 10002}};

  std::vector<std::array<std::vector<Int>, 3>> paramVec{
      {std::vector<Int>{-10003, -10002}, std::vector<Int>{0, 1},
       std::vector<Int>{1, 2}},
      {std::vector<Int>{-2, 2}, std::vector<Int>{2, 2}, std::vector<Int>{3, 5}},
      {std::vector<Int>{10000, 10002}, std::vector<Int>{1, 1},
       std::vector<Int>{2, 2}}};

  for (size_t b = 0; b < boundVec.size(); ++b) {
    auto const [lb, ub] = boundVec.at(b);
    EXPECT_LE(lb, ub);
    inputVarLb = lb;
    inputVarUb = ub;

    auto const [cover, low, up] = paramVec.at(b);

    coverSet.clear();

    for (size_t j = 0; j < cover.size(); ++j) {
      coverSet.emplace(cover.at(j), std::pair<Int, Int>(low.at(j), up.at(j)));
    }

    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const Int expectedOutput = computeOutput(ts);
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  numInputVars = 3;

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
    EXPECT_LE(lb, ub);
    inputVarLb = lb;
    inputVarUb = ub;

    auto const [cover, low, up] = paramVec[i];

    coverSet.clear();

    for (size_t j = 0; j < cover.size(); ++j) {
      coverSet.emplace(cover.at(j), std::pair<Int, Int>{low.at(j), up.at(j)});
    }

    Timestamp ts = _solver->currentTimestamp();

    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const Int expectedOutput = computeOutput(ts);
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, NextInput) {
  numInputVars = 1000;
  inputVarLb = 0;
  inputVarUb = numInputVars - 1;
  EXPECT_LE(inputVarLb, inputVarUb);

  coverSet.clear();
  for (Int i = 0; i < numInputVars; ++i) {
    coverSet.emplace(i, std::pair<Int, Int>{1, 2});
  }

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(GlobalCardinalityLowUpTest, NotifyCurrentInputChanged) {
  numInputVars = 100;
  inputVarLb = -10;
  inputVarUb = 10;

  coverSet.clear();
  for (Int i = 0; i < numInputVars; ++i) {
    coverSet.emplace(i, std::pair<Int, Int>{1, 2});
  }

  auto& invariant = generate();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, inputVarDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(GlobalCardinalityLowUpTest, Commit) {
  numInputVars = 1000;
  inputVarLb = -10;
  inputVarUb = 10;

  coverSet.clear();
  for (Int i = 0; i < numInputVars; ++i) {
    coverSet.emplace(i, std::pair<Int, Int>{1, 2});
  }

  auto& invariant = generate();

  std::vector<size_t> indices(numInputVars);
  std::iota(indices.begin(), indices.end(), 0);

  std::vector<Int> committedValues(numInputVars);

  for (Int i = 0; i < numInputVars; ++i) {
    committedValues.at(i) = _solver->committedValue(inputVars.at(i));
  }

  std::shuffle(indices.begin(), indices.end(), rng);

  EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
    for (Int j = 0; j < numInputVars; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputVars.at(j)),
                committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputVars.at(i), inputVarDist(gen));
    } while (oldVal == _solver->value(ts, inputVars.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedViolation = _solver->value(ts, outputVar);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedViolation, _solver->value(ts, outputVar));

    _solver->commitIf(ts, VarId(inputVars.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputVars.at(i)));
    _solver->commitIf(ts, VarId(outputVar));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedViolation, _solver->value(ts + 1, outputVar));
  }
}

RC_GTEST_FIXTURE_PROP(GlobalCardinalityLowUpTest, RapidCheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);

  inputVarLb = *rc::gen::inRange(
      std::numeric_limits<Int>::min() + (2 * numInputVars),
      std::numeric_limits<Int>::max() - (2 * numInputVars) + 1);

  inputVarUb = inputVarLb + numInputVars;

  const Int coverRange = 100;

  const Int c1 = *rc::gen::inRange(inputVarLb - numInputVars,
                                   inputVarUb + numInputVars + 1);
  const Int c2 = (std::numeric_limits<Int>::max() - coverRange < c1)
                     ? (c1 - coverRange)
                     : (c1 + coverRange);

  const Int coverLb = std::min(c1, c2);
  const Int coverUb = std::max(c1, c2);

  RC_ASSERT(coverLb + coverRange == coverUb);

  std::cerr << "numInputVars: " << numInputVars
            << "\ninputVarLb: " << inputVarLb << "\ninputVarUb: " << inputVarUb
            << "\ncoverLb: " << coverLb << "; coverUb: " << coverUb;

  std::vector<Int> cover(coverRange + 1);
  std::iota(cover.begin(), cover.end(), coverLb);
  RC_ASSERT(cover.front() == coverLb);
  RC_ASSERT(cover.back() == coverUb);
  std::shuffle(cover.begin(), cover.end(), rng);
  cover.resize(*rc::gen::inRange<size_t>(size_t{1}, cover.size() + 1));

  std::cerr << "\ncover: {";
  for (const auto& [c, p] : coverSet) {
    std::cerr << "<c: " << c << "; l: " << p.first << "; u: " << p.second
              << ">, ";
  }

  std::cerr << "}\n";

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (Int i = 0; i < numInputVars; ++i) {
        if (randBool()) {
          _solver->setValue(inputVars.at(i), inputVarDist(gen));
        }
      }

      _solver->endMove();

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      _solver->query(outputVar);
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(_solver->currentValue(outputVar) == computeOutput());
    }
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
  }
}

class MockGlobalCardinalityConst : public GlobalCardinalityLowUp {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    GlobalCardinalityLowUp::registerVars();
  }
  explicit MockGlobalCardinalityConst(SolverBase& solver, VarViewId outputVar,
                                      std::vector<VarViewId>&& inputVars,
                                      const std::vector<Int>& cover,
                                      const std::vector<Int>& counts)
      : GlobalCardinalityLowUp(solver, outputVar, std::move(inputVars), cover,
                               counts) {
    EXPECT_TRUE(outputVar.isVar());

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
