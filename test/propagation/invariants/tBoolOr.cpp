#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolOrTest : public InvariantTest {
 public:
  VarViewId x{NULL_ID};
  VarViewId y{NULL_ID};
  Int xLb{0};
  Int xUb{2};
  Int yLb{0};
  Int yUb{2};
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

  static Int computeOutput(Int xVal, Int yVal) { return std::min(xVal, yVal); }

  BoolOr& generate() {
    xDist = std::uniform_int_distribution<Int>(xLb, xUb);
    yDist = std::uniform_int_distribution<Int>(yLb, yUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    x = makeIntVar(xLb, xUb, xDist);
    y = makeIntVar(yLb, yUb, yDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    BoolOr& invariant =
        _solver->makeInvariant<BoolOr>(*_solver, outputVar, x, y);
    _solver->close();
    return invariant;
  }
};

TEST_F(BoolOrTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 1}, {1, 10}, {95, 100}};

  auto& invariant = generate();
  _solver->open();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_LE(xLb, xUb);
    _solver->updateBounds(VarId(x), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_LE(yLb, yUb);
      _solver->updateBounds(VarId(y), yLb, yUb, false);
      invariant.updateBounds(false);

      EXPECT_EQ(_solver->lowerBound(outputVar), std::min(xLb, yLb));
      EXPECT_EQ(_solver->upperBound(outputVar), std::min(xUb, yUb));
    }
  }
}

TEST_F(BoolOrTest, Recompute) {
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

TEST_F(BoolOrTest, NotifyInputChanged) {
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

TEST_F(BoolOrTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  expectNextInput(inputVars, invariant);
}

TEST_F(BoolOrTest, NotifyCurrentInputChanged) {
  xLb = 0;
  xUb = 10;
  yLb = xLb;
  yUb = xUb;

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

TEST_F(BoolOrTest, Commit) {
  xLb = 0;
  xUb = 2;
  yLb = xLb;
  yUb = xUb;
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  std::vector<Int> committedValues{_solver->committedValue(x),
                                   _solver->committedValue(y)};
  std::vector<size_t> indices{0, 1};
  std::shuffle(indices.begin(), indices.end(), rng);

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

RC_GTEST_FIXTURE_PROP(BoolOrTest, rapidcheck, ()) {
  xLb = *rc::gen::inRange<Int>(0, 3);
  xUb = *rc::gen::inRange<Int>(xLb, 3);
  yLb = *rc::gen::inRange<Int>(0, 3);
  yUb = *rc::gen::inRange<Int>(yLb, 3);

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

class MockBoolOr : public BoolOr {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolOr::registerVars();
  }
  explicit MockBoolOr(SolverBase& solver, VarViewId output, VarViewId x,
                      VarViewId y)
      : BoolOr(solver, output, x, y) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolOr::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolOr::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolOr::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolOr::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolOr::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BoolOrTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId x = _solver->makeIntVar(5, 0, 100);
    const VarViewId y = _solver->makeIntVar(0, 0, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockBoolOr>(
        &_solver->makeInvariant<MockBoolOr>(*_solver, output, x, y),
        {propMode, markingMode, 3, x, 1, output});
  }
}
}  // namespace atlantis::testing
