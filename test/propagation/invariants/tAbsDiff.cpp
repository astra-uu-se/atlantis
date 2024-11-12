#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/absDiff.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class AbsDiffTest : public InvariantTest {
 public:
  VarViewId x{NULL_ID};
  VarViewId y{NULL_ID};
  Int xLb{-5};
  Int xUb{5};
  Int yLb{-5};
  Int yUb{5};
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

  static Int computeOutput(const Int xVal, const Int yVal) {
    return std::abs(xVal - yVal);
  }

  AbsDiff& generate() {
    xDist = std::uniform_int_distribution<Int>(xLb, xUb);
    yDist = std::uniform_int_distribution<Int>(yLb, yUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    x = makeIntVar(xLb, xUb, xDist);
    y = makeIntVar(yLb, yUb, yDist);
    outputVar = _solver->makeIntVar(0, 0, 0);

    AbsDiff& invariant =
        _solver->makeInvariant<AbsDiff>(*_solver, outputVar, x, y);

    _solver->close();
    return invariant;
  }
};

TEST_F(AbsDiffTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  _solver->open();

  auto& invariant = generate();
  _solver->open();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_LE(xLb, xUb);
    _solver->updateBounds(VarId(x), xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_LE(yLb, yUb);
      _solver->updateBounds(VarId(y), yLb, yUb, false);
      invariant.updateBounds(false);
      std::vector<Int> outputs;
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        _solver->setValue(_solver->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          _solver->setValue(_solver->currentTimestamp(), y, yVal);
          invariant.updateBounds(false);
          invariant.recompute(_solver->currentTimestamp());
          outputs.emplace_back(_solver->currentValue(outputVar));
        }
      }
      const auto& [minViol, maxViol] =
          std::minmax_element(outputs.begin(), outputs.end());
      ASSERT_EQ(*minViol, _solver->lowerBound(outputVar));
      ASSERT_EQ(*maxViol, _solver->upperBound(outputVar));
    }
  }
}

TEST_F(AbsDiffTest, Recompute) {
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

TEST_F(AbsDiffTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  yLb = xLb;
  yLb = xUb;

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

TEST_F(AbsDiffTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  expectNextInput(inputVars, invariant);
}

TEST_F(AbsDiffTest, NotifyCurrentInputChanged) {
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

TEST_F(AbsDiffTest, Commit) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{x, y};

  std::vector<Int> committedValues(inputVars.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    committedValues.at(i) = _solver->committedValue(inputVars.at(i));
  }

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

RC_GTEST_FIXTURE_PROP(AbsDiffTest, rapidcheck, ()) {
  const Int xb1 = *rc::gen::arbitrary<Int>();
  const Int xb2 = *rc::gen::arbitrary<Int>();
  xLb = std::min(xb1, xb2);
  xUb = std::max(xb1, xb2);

  const Int yb1 = *rc::gen::arbitrary<Int>();
  const Int yb2 = *rc::gen::arbitrary<Int>();
  yLb = std::min(yb1, yb2);
  yUb = std::max(yb1, yb2);

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

class MockAbsDiff : public AbsDiff {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    AbsDiff::registerVars();
  }
  explicit MockAbsDiff(SolverBase& solver, VarViewId output, VarViewId x,
                       VarViewId y)
      : AbsDiff(solver, output, x, y) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return AbsDiff::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return AbsDiff::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          AbsDiff::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          AbsDiff::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      AbsDiff::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(AbsDiffTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId x = _solver->makeIntVar(-10, -100, 100);
    const VarViewId y = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockAbsDiff>(
        &_solver->makeInvariant<MockAbsDiff>(*_solver, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace atlantis::testing
