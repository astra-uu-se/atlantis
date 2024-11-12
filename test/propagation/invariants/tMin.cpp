#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/min.hpp"

namespace atlantis::testing {
using ::rc::gen::inRange;

using namespace atlantis::propagation;

class MinTest : public InvariantTest {
 public:
  Int numInputVars{3};
  Int inputVarLb{-2};
  Int inputVarUb{2};
  std::vector<VarViewId> inputVars;
  VarViewId outputVar{NULL_ID};
  std::uniform_int_distribution<Int> inputVarDist;

  void TearDown() override {
    InvariantTest::TearDown();
    inputVars.clear();
  }

  Int computeOutput(bool committedValue = false) {
    Int min_val = std::numeric_limits<Int>::max();
    for (auto var : inputVars) {
      min_val = std::min(min_val, committedValue ? _solver->committedValue(var)
                                                 : _solver->currentValue(var));
    }
    return min_val;
  }

  Int computeOutput(Timestamp ts) {
    Int min_val = std::numeric_limits<Int>::max();
    for (auto var : inputVars) {
      min_val = std::min(min_val, _solver->value(ts, var));
    }
    return min_val;
  }

  Min& generate() {
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
    Min& invariant = _solver->makeInvariant<Min>(
        *_solver, outputVar, std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }
};

TEST_F(MinTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{{0, 100}, {150, 250}};
  _solver->open();

  auto& invariant = generate();
  _solver->open();

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

        ASSERT_EQ(std::min(aLb, std::min(bLb, cLb)),
                  _solver->lowerBound(outputVar));
        ASSERT_EQ(std::min(aUb, std::min(bUb, cUb)),
                  _solver->upperBound(outputVar));
      }
    }
  }
}

TEST_F(MinTest, Recompute) {
  generateState = GenerateState::LB;

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

TEST_F(MinTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

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

TEST_F(MinTest, NextInput) {
  auto& invariant = generate();

  EXPECT_EQ(_solver->lowerBound(outputVar), inputVarLb);

  for (const auto& id : inputVars) {
    EXPECT_TRUE(id.isVar());
  }
  const auto minVarId = size_t(getMinVarViewId(inputVars));
  const auto maxVarId = size_t(getMaxVarViewId(inputVars));

  for (Int i = 0; i < numInputVars; ++i) {
    const Timestamp ts =
        _solver->currentTimestamp() + static_cast<Timestamp>(i);
    for (Int j = 0; j < numInputVars; ++j) {
      _solver->setValue(ts, inputVars.at(j), i == j ? inputVarLb : inputVarUb);
    }
    std::vector<bool> notified(maxVarId - minVarId + 1, false);
    for (Int j = 0; j <= i; ++j) {
      const size_t varId = size_t(invariant.nextInput(ts));
      EXPECT_NE(varId, NULL_ID);
      EXPECT_LE(minVarId, varId);
      EXPECT_GE(maxVarId, varId);
      EXPECT_FALSE(notified.at(varId - minVarId));
      notified.at(varId - minVarId) = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (Int j = 0; j <= i; ++j) {
      EXPECT_TRUE(notified.at(static_cast<size_t>(j) + minVarId));
    }
    for (Int j = i + 1; j < numInputVars; ++j) {
      EXPECT_FALSE(notified.at(static_cast<size_t>(j) + minVarId));
    }
  }
}

TEST_F(MinTest, NotifyCurrentInputChanged) {
  auto& invariant = generate();

  EXPECT_EQ(_solver->lowerBound(outputVar), inputVarLb);
  EXPECT_GE(inputVarUb - inputVarLb, 2);

  for (Int i = 0; i < numInputVars; ++i) {
    const Timestamp ts =
        _solver->currentTimestamp() + static_cast<Timestamp>(i);
    for (Int j = 0; j < numInputVars; ++j) {
      _solver->setValue(ts, inputVars.at(j), inputVarUb);
    }
    for (Int j = 0; j <= i; ++j) {
      EXPECT_EQ(invariant.nextInput(ts), inputVars.at(j));
      EXPECT_EQ(_solver->value(ts, inputVars.at(j)), inputVarUb);
      _solver->setValue(ts, inputVars.at(j),
                        i == j ? inputVarLb : (inputVarLb + 1));
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
  }
}

TEST_F(MinTest, Commit) {
  auto& invariant = generate();

  std::vector<size_t> indices(numInputVars);
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> committedValues(inputVars.size());
  for (size_t i = 0; i < inputVars.size(); ++i) {
    committedValues.at(i) = _solver->committedValue(inputVars.at(i));
  }

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

RC_GTEST_FIXTURE_PROP(MinTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (size_t i = 0; i < inputVars.size(); ++i) {
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

class MockExists : public Min {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Min::registerVars();
  }
  explicit MockExists(SolverBase& solver, VarViewId output,
                      std::vector<VarViewId>&& varArray)
      : Min(solver, output, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Min::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Min::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Min::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Min::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Min::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(MinTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<VarViewId> args;
    const Int numArgs = 10;
    for (Int value = 1; value <= numArgs; ++value) {
      args.push_back(_solver->makeIntVar(value, 1, numArgs));
    }
    const VarViewId modifiedVarId = args.front();
    const VarViewId output = _solver->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockExists>(
        &_solver->makeInvariant<MockExists>(*_solver, output, std::move(args)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
