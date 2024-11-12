#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/powDomain.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class PowDomainTest : public InvariantTest {
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

  Int computeOutput(Timestamp ts) {
    return computeOutput(_solver->value(ts, base),
                         _solver->value(ts, exponent));
  }

  Int computeOutput(bool committedValue = false) {
    return computeOutput(committedValue ? _solver->committedValue(base)
                                        : _solver->currentValue(base),
                         committedValue ? _solver->committedValue(exponent)
                                        : _solver->currentValue(exponent));
  }

  Int computeOutput(const Int xVal, const Int yVal) {
    return xVal == 0 && yVal < 0 ? 1 : 0;
  }

  PowDomain& generate() {
    baseDist = std::uniform_int_distribution<Int>(baseLb, baseUb);
    exponentDist = std::uniform_int_distribution<Int>(exponentLb, exponentUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    base = makeIntVar(baseLb, baseUb, baseDist);
    exponent = makeIntVar(exponentLb, exponentUb, exponentDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    PowDomain& invariant =
        _solver->makeInvariant<PowDomain>(*_solver, outputVar, base, exponent);
    _solver->close();
    return invariant;
  }
};

TEST_F(PowDomainTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, -10}, {-5, 0}, {-2, 2}, {0, 5}, {10, 10}, {15, 20}};

  auto& invariant = generate();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_LE(xLb, xUb);
    _solver->updateBounds(VarId(base), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_LE(yLb, yUb);
      _solver->updateBounds(VarId(exponent), yLb, yUb, false);
      invariant.updateBounds(false);
      std::vector<Int> violations;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), base, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          _solver->setValue(_solver->currentTimestamp(), exponent, yVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
          violations.emplace_back(_solver->currentValue(outputVar));
        }
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(violations.begin(), violations.end());
      ASSERT_EQ(*minViol, _solver->lowerBound(outputVar));
      if (*maxViol != _solver->upperBound(outputVar)) {
        ASSERT_EQ(*maxViol, _solver->upperBound(outputVar));
      }
    }
  }
}

TEST_F(PowDomainTest, Recompute) {
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

TEST_F(PowDomainTest, NotifyInputChanged) {
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

TEST_F(PowDomainTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{base, exponent};

  expectNextInput(inputVars, invariant);
}

TEST_F(PowDomainTest, NotifyCurrentInputChanged) {
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

TEST_F(PowDomainTest, Commit) {
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

RC_GTEST_FIXTURE_PROP(PowDomainTest, rapidcheck, ()) {
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

class MockPowDomain : public PowDomain {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    PowDomain::registerVars();
  }
  explicit MockPowDomain(SolverBase& solver, VarViewId outputVar,
                         VarViewId base, VarViewId exponent)
      : PowDomain(solver, outputVar, base, exponent) {
    EXPECT_TRUE(outputVar.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return PowDomain::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return PowDomain::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          PowDomain::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          PowDomain::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      PowDomain::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(PowDomainTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    base = _solver->makeIntVar(5, -100, 100);
    exponent = _solver->makeIntVar(0, -100, 100);
    const VarViewId viol = _solver->makeIntVar(0, 0, 1);
    testNotifications<MockPowDomain>(
        &_solver->makeViolationInvariant<MockPowDomain>(*_solver, viol, base,
                                                        exponent),
        {propMode, markingMode, 3, base, 0, viol});
  }
}

}  // namespace atlantis::testing
