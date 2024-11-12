#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolAllEqualTest : public InvariantTest {
 public:
  Int numInputVars{3};
  std::vector<VarViewId> inputVars;
  VarViewId outputVar{NULL_ID};
  Int inputVarLb{-10};
  Int inputVarUb{10};
  std::uniform_int_distribution<Int> inputVarDist;

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

  static Int computeOutput(const std::vector<Int>& values) {
    size_t numFalse = 0;
    size_t numTrue = 0;
    for (const Int val : values) {
      if (val == 0) {
        ++numTrue;
      } else {
        ++numFalse;
      }
    }
    return static_cast<Int>(
        std::min(values.size() - numTrue, values.size() - numFalse));
  }

  BoolAllEqual& generate() {
    inputVars.clear();
    inputVars.reserve(numInputVars);
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(
          _solver->makeIntVar(inputVarDist(gen), inputVarLb, inputVarUb));
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    BoolAllEqual& invariant = _solver->makeViolationInvariant<BoolAllEqual>(
        *_solver, outputVar, std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }
};

TEST_F(BoolAllEqualTest, UpdateBounds) {}

TEST_F(BoolAllEqualTest, Recompute) {
  generateState = GenerateState::LB;

  numInputVars = 3;
  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 5}, {10, 10}};

  for (const auto& [lb, ub] : boundVec) {
    inputVarLb = lb;
    inputVarUb = ub;

    auto& invariant = generate();

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
}

TEST_F(BoolAllEqualTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  std::vector<std::pair<Int, Int>> boundVec{{0, 0}, {0, 5}, {10, 10}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_LE(lb, ub);

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

TEST_F(BoolAllEqualTest, NextInput) {
  numInputVars = 1000;
  inputVarLb = 0;
  inputVarUb = numInputVars - 1;

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(BoolAllEqualTest, NotifyCurrentInputChanged) {
  inputVarLb = 0;
  inputVarUb = 10;

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

TEST_F(BoolAllEqualTest, Commit) {
  numInputVars = 1000;
  inputVarLb = 0;
  inputVarUb = 10;

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

RC_GTEST_FIXTURE_PROP(BoolAllEqualTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);

  inputVarLb = 0;
  inputVarUb = 1;

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

class MockAllDifferent : public BoolAllEqual {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolAllEqual::registerVars();
  }
  explicit MockAllDifferent(SolverBase& solver, VarViewId outputVar,
                            std::vector<VarViewId>&& t_vars)
      : BoolAllEqual(solver, outputVar, std::move(t_vars)) {
    EXPECT_TRUE(outputVar.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolAllEqual::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolAllEqual::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolAllEqual::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolAllEqual::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolAllEqual::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(BoolAllEqualTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<VarViewId> args;
    const size_t numArgs = 10;
    for (size_t value = 0; value < numArgs; ++value) {
      args.emplace_back(_solver->makeIntVar(0, -100, 100));
    }
    const VarViewId viol = _solver->makeIntVar(0, 0, static_cast<Int>(numArgs));
    const VarViewId modifiedVarId = args.front();
    testNotifications<MockAllDifferent>(
        &_solver->makeViolationInvariant<MockAllDifferent>(*_solver, viol,
                                                           std::move(args)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 1, viol});
  }
}

}  // namespace atlantis::testing
