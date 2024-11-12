#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/ifThenElse.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IfThenElseTest : public InvariantTest {
 public:
  VarViewId conditionVar{NULL_ID};
  VarViewId thenVar{NULL_ID};
  VarViewId elseVar{NULL_ID};
  Int conditionLb{0};
  Int conditionUb{1};
  Int thenLb{-3};
  Int thenUb{-1};
  Int elseLb{1};
  Int elseUb{3};
  VarViewId outputVar{NULL_ID};

  std::uniform_int_distribution<Int> conditionDist;
  std::uniform_int_distribution<Int> thenDist;
  std::uniform_int_distribution<Int> elseDist;

  Int computeOutput(Timestamp ts) {
    return computeOutput(_solver->value(ts, conditionVar),
                         _solver->value(ts, thenVar),
                         _solver->value(ts, elseVar));
  }

  Int computeOutput(bool committedValue = false) {
    return computeOutput(committedValue ? _solver->committedValue(conditionVar)
                                        : _solver->currentValue(conditionVar),
                         committedValue ? _solver->committedValue(thenVar)
                                        : _solver->currentValue(thenVar),
                         committedValue ? _solver->committedValue(elseVar)
                                        : _solver->currentValue(elseVar));
  }

  static Int computeOutput(Int conditionVal, Int thenVal, Int elseVal) {
    return conditionVal == 0 ? thenVal : elseVal;
  }

  IfThenElse& generate() {
    conditionDist =
        std::uniform_int_distribution<Int>(conditionLb, conditionUb);
    thenDist = std::uniform_int_distribution<Int>(thenLb, thenUb);
    elseDist = std::uniform_int_distribution<Int>(elseLb, elseUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    conditionVar = makeIntVar(conditionLb, conditionUb, conditionDist);
    thenVar = makeIntVar(thenLb, thenUb, thenDist);
    elseVar = makeIntVar(elseLb, elseUb, elseDist);
    outputVar = _solver->makeIntVar(0, 0, 0);
    IfThenElse& invariant = _solver->makeInvariant<IfThenElse>(
        *_solver, outputVar, conditionVar, thenVar, elseVar);
    _solver->close();
    return invariant;
  }
};

TEST_F(IfThenElseTest, UpdateBounds) {
  auto& invariant = generate();

  std::vector<std::pair<Int, Int>> cBounds{{0, 0}, {0, 100}, {1, 10000}};

  for (const auto& [cLb, cUb] : cBounds) {
    EXPECT_LE(cLb, cUb);
    _solver->updateBounds(VarId(conditionVar), cLb, cUb, false);
    invariant.updateBounds(false);
    if (cLb == 0 && cUb == 0) {
      EXPECT_EQ(_solver->lowerBound(outputVar), _solver->lowerBound(thenVar));
      EXPECT_EQ(_solver->upperBound(outputVar), _solver->upperBound(thenVar));
    } else if (cLb > 0) {
      EXPECT_EQ(_solver->lowerBound(outputVar), _solver->lowerBound(elseVar));
      EXPECT_EQ(_solver->upperBound(outputVar), _solver->upperBound(elseVar));
    } else {
      EXPECT_EQ(
          _solver->lowerBound(outputVar),
          std::min(_solver->lowerBound(thenVar), _solver->lowerBound(elseVar)));
      EXPECT_EQ(
          _solver->upperBound(outputVar),
          std::max(_solver->upperBound(thenVar), _solver->upperBound(elseVar)));
    }
  }
}

TEST_F(IfThenElseTest, Recompute) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{conditionVar, thenVar, elseVar};

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

TEST_F(IfThenElseTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{conditionVar, thenVar, elseVar};

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

TEST_F(IfThenElseTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{conditionVar, thenVar, elseVar};

  const size_t minVarId = size_t(getMinVarViewId(inputVars));
  const size_t maxVarId = size_t(getMaxVarViewId(inputVars));

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId - minVarId + 1, false);
    // First input is conditionVar,
    // Second input is thenVar if conditionVar = 0, otherwise elseVar:
    for (size_t i = 0; i < 2; ++i) {
      const size_t varId = size_t(invariant.nextInput(ts));
      EXPECT_NE(varId, NULL_ID);
      EXPECT_LE(minVarId, varId);
      EXPECT_GE(maxVarId, varId);
      EXPECT_FALSE(notified.at(varId - minVarId));
      notified.at(varId - minVarId) = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    const Int conditionVal = _solver->value(ts, conditionVar);

    EXPECT_TRUE(notified.at(size_t(conditionVar)));
    EXPECT_TRUE(notified.at(size_t(conditionVal == 0 ? thenVar : elseVar)));
    EXPECT_FALSE(notified.at(size_t(conditionVal != 0 ? thenVar : elseVar)));
  }
}

TEST_F(IfThenElseTest, NotifyCurrentInputChanged) {
  auto& invariant = generate();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (size_t i = 0; i < 2; ++i) {
      const Int conditionVal = _solver->value(ts, conditionVar);
      const VarViewId curInput = invariant.nextInput(ts);
      EXPECT_EQ(curInput, i == 0 ? conditionVar
                                 : (conditionVal == 0 ? thenVar : elseVar));

      const Int oldVal = _solver->value(ts, curInput);
      do {
        _solver->setValue(ts, curInput,
                          i == 0 ? conditionDist(gen)
                                 : (i == 1 ? thenDist(gen) : elseDist(gen)));
      } while (_solver->value(ts, curInput) == oldVal);

      invariant.notifyCurrentInputChanged(ts);

      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(IfThenElseTest, Commit) {
  auto& invariant = generate();
  std::vector<VarViewId> inputVars{conditionVar, thenVar, elseVar};

  std::vector<size_t> indices(inputVars.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> committedValues(inputVars.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    committedValues.at(i) = _solver->committedValue(inputVars.at(i));
  }

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
                        i == 0 ? conditionDist(gen)
                               : (i == 1 ? thenDist(gen) : elseDist(gen)));
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

RC_GTEST_FIXTURE_PROP(IfThenElseTest, rapidcheck, ()) {
  const Int c1 = *rc::gen::arbitrary<Int>();
  const Int c2 = *rc::gen::arbitrary<Int>();
  conditionLb = std::min(c1, c2);
  conditionUb = std::max(c1, c2);

  const Int t1 = *rc::gen::arbitrary<Int>();
  const Int t2 = *rc::gen::arbitrary<Int>();
  thenLb = std::min(t1, t2);
  thenUb = std::max(t1, t2);

  const Int e1 = *rc::gen::arbitrary<Int>();
  const Int e2 = *rc::gen::arbitrary<Int>();
  elseLb = std::min(e1, e2);
  elseUb = std::max(e1, e2);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(conditionVar, conditionDist(gen));
      }
      if (randBool()) {
        _solver->setValue(thenVar, thenDist(gen));
      }
      if (randBool()) {
        _solver->setValue(elseVar, elseDist(gen));
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

class MockIfThenElse : public IfThenElse {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    IfThenElse::registerVars();
  }
  explicit MockIfThenElse(SolverBase& solver, VarViewId output, VarViewId b,
                          VarViewId x, VarViewId y)
      : IfThenElse(solver, output, b, x, y) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return IfThenElse::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return IfThenElse::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          IfThenElse::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          IfThenElse::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      IfThenElse::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(IfThenElseTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId b = _solver->makeIntVar(0, -100, 100);
    const VarViewId x = _solver->makeIntVar(0, 0, 4);
    const VarViewId y = _solver->makeIntVar(5, 5, 9);
    const VarViewId output = _solver->makeIntVar(3, 0, 9);
    testNotifications<MockIfThenElse>(
        &_solver->makeInvariant<MockIfThenElse>(*_solver, output, b, x, y),
        {propMode, markingMode, 3, b, 5, output});
  }
}

}  // namespace atlantis::testing
