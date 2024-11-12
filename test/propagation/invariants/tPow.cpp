#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/pow.hpp"
#include "atlantis/utils/pow.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class PowTest : public InvariantTest {
 public:
  VarViewId base{NULL_ID};
  VarViewId exponent{NULL_ID};
  Int baseLb{-2};
  Int baseUb{2};
  Int exponentLb{-2};
  Int exponentUb{2};
  VarViewId outputVar{NULL_ID};

  std::uniform_int_distribution<Int> baseDist;
  std::uniform_int_distribution<Int> exponentDist;

  Pow& generate() {
    baseDist = std::uniform_int_distribution<Int>(baseLb, baseUb);
    exponentDist = std::uniform_int_distribution<Int>(exponentLb, exponentUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    base = makeIntVar(baseLb, baseUb, baseDist);
    exponent = makeIntVar(exponentLb, exponentUb, exponentDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    Pow& invariant =
        _solver->makeInvariant<Pow>(*_solver, outputVar, base, exponent);
    _solver->close();
    return invariant;
  }

  Int zeroReplacement() const {
    // base is always negative
    return baseLb < 0 && baseUb <= 0 ? -1 : 1;
  }

  Int computeOutput(bool committedValue = false) {
    return computeOutput(committedValue ? _solver->committedValue(base)
                                        : _solver->currentValue(base),
                         committedValue ? _solver->committedValue(exponent)
                                        : _solver->currentValue(exponent));
  }

  Int computeOutput(Timestamp ts) {
    return computeOutput(_solver->value(ts, base),
                         _solver->value(ts, exponent));
  }

  Int computeOutput(Int baseVal, Int expVal) {
    return pow_zero_replacement(baseVal, expVal, zeroReplacement());
  }
};

TEST_F(PowTest, UpdateBounds) {
  auto& invariant = generate();

  std::vector<std::pair<Int, Int>> boundVec{
      {-8, -5}, {-3, 0}, {-2, 2}, {0, 3}, {5, 8}};

  for (const auto& [baseLb, baseUb] : boundVec) {
    EXPECT_LE(baseLb, baseUb);
    _solver->updateBounds(VarId(base), baseLb, baseUb, false);
    for (const auto& [expLb, expUb] : boundVec) {
      EXPECT_LE(expLb, expUb);
      _solver->updateBounds(VarId(exponent), expLb, expUb, false);
      _solver->open();
      _solver->close();
      for (Int baseVal = baseLb; baseVal <= baseUb; ++baseVal) {
        _solver->setValue(_solver->currentTimestamp(), base, baseVal);
        for (Int expVal = expLb; expVal <= expUb; ++expVal) {
          _solver->setValue(_solver->currentTimestamp(), exponent, expVal);
          invariant.recompute(_solver->currentTimestamp());
          const Int o = _solver->currentValue(outputVar);
          if (o < _solver->lowerBound(outputVar) ||
              _solver->upperBound(outputVar) < o) {
            invariant.updateBounds(false);
            ASSERT_GE(o, _solver->lowerBound(outputVar));
            ASSERT_LE(o, _solver->upperBound(outputVar));
          }
        }
      }
    }
  }
}

TEST_F(PowTest, Recompute) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{base, exponent};

  auto inputVals = makeValVector(inputVars);

  Timestamp ts = _solver->currentTimestamp();

  while (increaseNextVal(inputVars, inputVals) >= 0) {
    ++ts;
    setVarVals(ts, inputVars, inputVals);

    const Int expectedOutput = computeOutput(ts);
    invariant.recompute(ts);
    EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
  }
}

TEST_F(PowTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{base, exponent};

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

TEST_F(PowTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{base, exponent};

  expectNextInput(inputVars, invariant);
}

TEST_F(PowTest, NotifyCurrentInputChanged) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{base, exponent};

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId,
                          varId == base ? baseDist(gen) : exponentDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(PowTest, Commit) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{base, exponent};

  std::vector<size_t> indices{0, 1};
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> committedValues{_solver->committedValue(base),
                                   _solver->committedValue(exponent)};

  EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputVars.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputVars.at(j)),
                committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputVars.at(i),
                        i == 0 ? baseDist(gen) : exponentDist(gen));
    } while (oldVal == _solver->value(ts, inputVars.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = _solver->value(ts, outputVar);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

    _solver->commitIf(ts, VarId(inputVars.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputVars.at(i)));
    _solver->commitIf(ts, VarId(outputVar));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
  }
}

RC_GTEST_FIXTURE_PROP(PowTest, rapidcheck, ()) {
  _solver->open();

  const Int b1 = *rc::gen::inRange<Int>(-10, 11);
  const Int b2 = *rc::gen::inRange<Int>(-10, 11);
  baseLb = std::min(b1, b2);
  baseUb = std::max(b1, b2);

  const Int e1 = *rc::gen::inRange<Int>(-10, 11);
  const Int e2 = *rc::gen::inRange<Int>(-10, 11);

  exponentLb = std::min(e1, e2);
  exponentUb = std::max(e1, e2);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(base, baseDist(gen));
      }
      if (randBool()) {
        _solver->setValue(exponent, exponentDist(gen));
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

class MockPow : public Pow {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Pow::registerVars();
  }
  explicit MockPow(SolverBase& solver, VarViewId output, VarViewId base,
                   VarViewId exponent)
      : Pow(solver, output, base, exponent) {
    EXPECT_TRUE(output.isVar());

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
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(PowTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId base = _solver->makeIntVar(-10, -100, 100);
    const VarViewId exponent = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockPow>(
        &_solver->makeInvariant<MockPow>(*_solver, output, base, exponent),
        {propMode, markingMode, 3, base, 0, output});
  }
}

}  // namespace atlantis::testing
