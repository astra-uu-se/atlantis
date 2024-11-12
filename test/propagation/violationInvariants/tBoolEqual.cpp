#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolEqualTest : public InvariantTest {
 public:
  VarViewId x{NULL_ID};
  VarViewId y{NULL_ID};
  Int xLb{0};
  Int xUb{1};
  Int yLb{0};
  Int yUb{1};
  VarViewId outputVar{NULL_ID};

  std::uniform_int_distribution<Int> xDist;
  std::uniform_int_distribution<Int> yDist;

  Int computeOutput(Timestamp ts) {
    return computeOutput(_solver->value(ts, x), _solver->value(ts, y));
  }

  Int computeOutput(bool committedValue = false) {
    return computeOutput(
        committedValue ? _solver->committedValue(x) : _solver->currentValue(x),
        committedValue ? _solver->committedValue(y) : _solver->currentValue(y));
  }

  static Int computeOutput(Int xVal, Int yVal) {
    return ((xVal == 0) == (yVal == 0)) ? 0 : 1;
  }

  BoolEqual& generate() {
    xDist = std::uniform_int_distribution<Int>(xLb, xUb);
    yDist = std::uniform_int_distribution<Int>(yLb, yUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    x = makeIntVar(xLb, xUb, xDist);
    y = makeIntVar(yLb, yUb, yDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    BoolEqual& invariant =
        _solver->makeInvariant<BoolEqual>(*_solver, outputVar, x, y);
    _solver->close();
    return invariant;
  }
};

TEST_F(BoolEqualTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 10}, {1, 10}, {10, 100}};
  auto& invariant = generate();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_LE(xLb, xUb);
    _solver->updateBounds(VarId(x), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_LE(yLb, yUb);
      _solver->updateBounds(VarId(y), yLb, yUb, false);
      invariant.updateBounds(false);
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          _solver->setValue(_solver->currentTimestamp(), y, yVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
        }
      }
      ASSERT_GE(0, _solver->lowerBound(outputVar));
      ASSERT_LE(1, _solver->upperBound(outputVar));
    }
  }
}

TEST_F(BoolEqualTest, Recompute) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

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

TEST_F(BoolEqualTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

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

TEST_F(BoolEqualTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  expectNextInput(inputVars, invariant);
}

TEST_F(BoolEqualTest, NotifyCurrentInputChanged) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, varId == x ? xDist(gen) : yDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(BoolEqualTest, Commit) {
  xLb = 0;
  xUb = 10;

  yLb = 0;
  yUb = 10;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  std::vector<size_t> indices{0, 1};
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> committedValues{_solver->committedValue(x),
                                   _solver->committedValue(y)};

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
      _solver->setValue(ts, inputVars.at(i), i == 0 ? xDist(gen) : yDist(gen));
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

RC_GTEST_FIXTURE_PROP(BoolEqualTest, rapidcheck, ()) {
  _solver->open();

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(x, xDist(gen));
      }
      if (randBool()) {
        _solver->setValue(y, yDist(gen));
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

class MockBoolEqual : public BoolEqual {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolEqual::registerVars();
  }
  explicit MockBoolEqual(SolverBase& solver, VarViewId outputVar, VarViewId x,
                         VarViewId y)
      : BoolEqual(solver, outputVar, x, y) {
    EXPECT_TRUE(outputVar.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolEqual::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolEqual::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolEqual::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolEqual::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolEqual::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(BoolEqualTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    x = _solver->makeIntVar(5, 0, 100);
    y = _solver->makeIntVar(0, 0, 100);
    const VarViewId viol = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockBoolEqual>(
        &_solver->makeViolationInvariant<MockBoolEqual>(*_solver, viol, x, y),
        {propMode, markingMode, 3, x, 1, viol});
  }
}
}  // namespace atlantis::testing
