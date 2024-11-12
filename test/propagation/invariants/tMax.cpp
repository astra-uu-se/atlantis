#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/max.hpp"

namespace atlantis::testing {
using ::rc::gen::inRange;

using namespace atlantis::propagation;

class MaxTest : public InvariantTest {
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
    Int maxVal = std::numeric_limits<Int>::min();
    for (auto var : inputVars) {
      maxVal = std::max(maxVal, committedValue ? _solver->committedValue(var)
                                               : _solver->currentValue(var));
    }
    return maxVal;
  }

  Int computeOutput(Timestamp ts) {
    Int maxVal = std::numeric_limits<Int>::min();
    for (auto var : inputVars) {
      maxVal = std::max(maxVal, _solver->value(ts, var));
    }
    return maxVal;
  }

  Max& generate() {
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
    Max& invariant = _solver->makeInvariant<Max>(
        *_solver, outputVar, std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }
};

TEST_F(MaxTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-100, -100}, {-100, 0}, {0, 0}, {0, 100}, {100, 100}};
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

        ASSERT_EQ(std::max(aLb, std::max(bLb, cLb)),
                  _solver->lowerBound(outputVar));
        ASSERT_EQ(std::max(aUb, std::max(bUb, cUb)),
                  _solver->upperBound(outputVar));
      }
    }
  }
}

TEST_F(MaxTest, Recompute) {
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

TEST_F(MaxTest, NotifyInputChanged) {
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

TEST_F(MaxTest, NextInput) {
  auto& invariant = generate();

  EXPECT_EQ(_solver->upperBound(outputVar), inputVarUb);

  for (const auto& id : inputVars) {
    EXPECT_TRUE(id.isVar());
  }
  const auto minVarId = size_t(getMinVarViewId(inputVars));
  const auto maxVarId = size_t(getMaxVarViewId(inputVars));

  for (Int i = 0; i < numInputVars; ++i) {
    const Timestamp ts =
        _solver->currentTimestamp() + static_cast<Timestamp>(i);
    for (Int j = 0; j < numInputVars; ++j) {
      _solver->setValue(ts, inputVars.at(j), i == j ? inputVarUb : inputVarLb);
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

TEST_F(MaxTest, NotifyCurrentInputChanged) {
  auto& invariant = generate();

  EXPECT_EQ(_solver->upperBound(outputVar), inputVarUb);
  EXPECT_GE(inputVarUb - inputVarLb, 2);

  for (Int i = 0; i < numInputVars; ++i) {
    const Timestamp ts =
        _solver->currentTimestamp() + static_cast<Timestamp>(i);
    for (Int j = 0; j < numInputVars; ++j) {
      _solver->setValue(ts, inputVars.at(j), inputVarLb);
    }
    for (Int j = 0; j <= i; ++j) {
      EXPECT_EQ(invariant.nextInput(ts), inputVars.at(j));
      EXPECT_EQ(_solver->value(ts, inputVars.at(j)), inputVarLb);
      _solver->setValue(ts, inputVars.at(j),
                        i == j ? inputVarUb : (inputVarUb - 1));
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
  }
}

TEST_F(MaxTest, Commit) {
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

RC_GTEST_FIXTURE_PROP(MaxTest, rapidcheck, ()) {
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

class MockExists : public Max {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Max::registerVars();
  }
  explicit MockExists(SolverBase& solver, VarViewId output,
                      std::vector<VarViewId>&& varArray)
      : Max(solver, output, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Max::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Max::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Max::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Max::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Max::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(MaxTest, SolverIntegration) {
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
