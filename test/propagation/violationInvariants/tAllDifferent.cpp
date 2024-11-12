#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class AllDifferentTest : public InvariantTest {
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
    std::unordered_map<Int, Int> valueCounts;
    valueCounts.reserve(values.size());
    for (const Int value : values) {
      if (valueCounts.contains(value)) {
        valueCounts.at(value) += 1;
      } else {
        valueCounts.emplace(value, 1);
      }
    }
    Int expectedViolation = 0;
    for (const auto& [_, count] : valueCounts) {
      if (count > 1) {
        expectedViolation += count - 1;
      }
    }
    return expectedViolation;
  }

  AllDifferent& generate() {
    inputVars.clear();
    inputVars.reserve(numInputVars);
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(makeIntVar(inputVarLb, inputVarUb, inputVarDist));
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    AllDifferent& invariant = _solver->makeViolationInvariant<AllDifferent>(
        *_solver, outputVar, std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }
};

TEST_F(AllDifferentTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-250, -150}, {-100, 0}, {-50, 50}, {0, 100}, {150, 250}};

  auto& invariant = generate();

  for (const auto& [aLb, aUb] : boundVec) {
    EXPECT_LE(aLb, aUb);
    _solver->updateBounds(VarId(inputVars.at(0)), aLb, aUb, false);
    for (const auto& [bLb, bUb] : boundVec) {
      EXPECT_LE(bLb, bUb);
      _solver->updateBounds(VarId(inputVars.at(1)), bLb, bUb, false);
      for (const auto& [cLb, cUb] : boundVec) {
        EXPECT_LE(cLb, cUb);
        _solver->updateBounds(VarId(inputVars.at(2)), cLb, cUb, false);
        invariant.updateBounds(false);
        ASSERT_EQ(0, _solver->lowerBound(outputVar));
        ASSERT_EQ(inputVars.size() - 1, _solver->upperBound(outputVar));
      }
    }
  }
}

TEST_F(AllDifferentTest, Recompute) {
  generateState = GenerateState::LB;

  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_LE(lb, ub);
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

TEST_F(AllDifferentTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  for (const auto& [lb, ub] : boundVec) {
    EXPECT_LE(lb, ub);

    inputVarLb = lb;
    inputVarUb = ub;
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

TEST_F(AllDifferentTest, NextInput) {
  numInputVars = 100;
  inputVarLb = -10;
  inputVarUb = 10;

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(AllDifferentTest, NotifyCurrentInputChanged) {
  numInputVars = 100;
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

TEST_F(AllDifferentTest, Commit) {
  numInputVars = 1000;

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

RC_GTEST_FIXTURE_PROP(AllDifferentTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);

  inputVarLb =
      *rc::gen::inRange(std::numeric_limits<Int>::min() + numInputVars,
                        std::numeric_limits<Int>::max() - numInputVars);

  inputVarUb = inputVarLb + numInputVars;

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

class MockAllDifferent : public AllDifferent {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    AllDifferent::registerVars();
  }
  explicit MockAllDifferent(SolverBase& solver, VarViewId outputVar,
                            std::vector<VarViewId>&& t_vars)
      : AllDifferent(solver, outputVar, std::move(t_vars)) {
    EXPECT_TRUE(outputVar.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return AllDifferent::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return AllDifferent::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          AllDifferent::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          AllDifferent::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      AllDifferent::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(AllDifferentTest, SolverIntegration) {
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
