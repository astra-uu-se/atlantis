#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/exists.hpp"

namespace atlantis::testing {
using ::rc::gen::inRange;

using namespace atlantis::propagation;

class ExistsTest : public InvariantTest {
 public:
  Int numInputVars{3};
  Int inputVarLb{0};
  Int inputVarUb{5};
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

  Int computeOutput(const std::vector<Int>& values) {
    return *std::min_element(values.begin(), values.end());
  }

  Exists& generate() {
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
    Exists& invariant = _solver->makeInvariant<Exists>(
        *_solver, outputVar, std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }
};

TEST_F(ExistsTest, UpdateBounds) {
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

TEST_F(ExistsTest, Recompute) {
  generateState = GenerateState::LB;

  const Int iLb = 0;
  const Int iUb = 20;

  ASSERT_TRUE(iLb <= iUb);

  std::uniform_int_distribution<Int> iDist(iLb, iUb);

  _solver->open();

  auto inputVars = makeVars(2, 0, 2);

  const VarViewId outputVar = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

  Exists& invariant = _solver->makeInvariant<Exists>(
      *_solver, outputVar, std::vector<VarViewId>(inputVars));
  _solver->close();

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

TEST_F(ExistsTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  auto inputVals = makeValVector(inputVars);

  Timestamp ts = _solver->currentTimestamp();

  Int i{-1};

  while ((i = increaseNextVal(inputVars, inputVals)) >= 0) {
    ++ts;
    setVarVals(ts, inputVars, inputVals);

    const Int expectedOutput = computeOutput(ts);
    invariant.notifyInputChanged(ts, LocalId(i));
    EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
  }
}

TEST_F(ExistsTest, NextInput) {
  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(ExistsTest, NotifyCurrentInputChanged) {
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

TEST_F(ExistsTest, Commit) {
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

RC_GTEST_FIXTURE_PROP(ExistsTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 10;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (size_t i = 0; i < inputVars.size(); ++i) {
        if (*rc::gen::arbitrary<bool>()) {
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

class MockExists : public Exists {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Exists::registerVars();
  }
  explicit MockExists(SolverBase& solver, VarViewId output,
                      std::vector<VarViewId>&& varArray)
      : Exists(solver, output, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Exists::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Exists::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Exists::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Exists::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Exists::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ExistsTest, SolverIntegration) {
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
