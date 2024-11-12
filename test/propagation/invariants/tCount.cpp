#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/count.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class CountTest : public InvariantTest {
 public:
  Int haystackSize{3};
  std::vector<VarViewId> haystackVars;
  VarViewId needleVar{NULL_ID};
  VarViewId outputVar{NULL_ID};
  Int haystackVarLb{-10};
  Int haystackVarUb{10};
  Int needleVarLb{-10};
  Int needleVarUb{10};
  std::uniform_int_distribution<Int> haystackVarDist;
  std::uniform_int_distribution<Int> needleVarDist;

  Count& generate(bool generateInputVars = true) {
    haystackVarDist =
        std::uniform_int_distribution<Int>(haystackVarLb, haystackVarUb);
    needleVarDist =
        std::uniform_int_distribution<Int>(needleVarLb, needleVarUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    if (generateInputVars) {
      haystackVars.clear();
      haystackVars.reserve(haystackSize);

      for (Int i = 0; i < haystackSize; ++i) {
        haystackVars.emplace_back(
            makeIntVar(haystackVarLb, haystackVarUb, haystackVarDist));
      }
      needleVar = makeIntVar(needleVarLb, needleVarUb, needleVarDist);
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    Count& invariant = _solver->makeInvariant<Count>(
        *_solver, outputVar, needleVar, std::vector<VarViewId>(haystackVars));

    _solver->close();
    return invariant;
  }

  Int computeOutput(Timestamp ts) {
    std::vector<Int> values(haystackVars.size(), 0);
    for (size_t i = 0; i < haystackVars.size(); ++i) {
      values.at(i) = _solver->value(ts, haystackVars.at(i));
    }
    return computeOutput(_solver->value(ts, needleVar), values);
  }

  Int computeOutput(bool committedValue = false) {
    std::vector<Int> values(haystackVars.size(), 0);
    for (size_t i = 0; i < haystackVars.size(); ++i) {
      values.at(i) = committedValue
                         ? _solver->committedValue(haystackVars.at(i))
                         : _solver->currentValue(haystackVars.at(i));
    }
    return computeOutput(committedValue ? _solver->committedValue(needleVar)
                                        : _solver->currentValue(needleVar),
                         values);
  }

  Int computeOutput(Int needleVal, const std::vector<Int>& values) {
    Int count = 0;
    for (Int value : values) {
      if (value == needleVal) {
        ++count;
      }
    }
    return count;
  }
};

TEST_F(CountTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, 0}, {-5, 5}, {0, 10}, {15, 20}};
  _solver->open();

  haystackVars = std::vector<VarViewId>{_solver->makeIntVar(0, 0, 10),
                                        _solver->makeIntVar(0, 0, 10),
                                        _solver->makeIntVar(0, 0, 10)};
  needleVar = _solver->makeIntVar(0, 0, 10);

  auto& invariant = generate(false);

  for (const auto& [yLb, yUb] : boundVec) {
    EXPECT_LE(yLb, yUb);
    _solver->updateBounds(VarId(needleVar), yLb, yUb, false);
    for (const auto& [aLb, aUb] : boundVec) {
      EXPECT_LE(aLb, aUb);
      _solver->updateBounds(VarId(haystackVars.at(0)), aLb, aUb, false);
      for (const auto& [bLb, bUb] : boundVec) {
        EXPECT_LE(bLb, bUb);
        _solver->updateBounds(VarId(haystackVars.at(1)), bLb, bUb, false);
        for (const auto& [cLb, cUb] : boundVec) {
          EXPECT_LE(cLb, cUb);
          _solver->updateBounds(VarId(haystackVars.at(2)), cLb, cUb, false);
          invariant.updateBounds(false);

          ASSERT_GE(0, _solver->lowerBound(outputVar));
          ASSERT_LE(haystackVars.size(), _solver->upperBound(outputVar));
        }
      }
    }
  }
}

TEST_F(CountTest, Recompute) {
  generateState = GenerateState::LB;

  haystackSize = 3;

  std::pair<Int, Int> inputBound{-1, 1};
  std::vector<std::pair<Int, Int>> haystackBounds(haystackSize, inputBound);

  _solver->open();

  needleVar = _solver->makeIntVar(inputBound.first, inputBound.first,
                                  inputBound.second);

  haystackVars = makeVars(haystackBounds);

  auto& invariant = generate(false);

  // add needle to inputs:
  std::vector<VarViewId> inputVars(haystackVars);
  inputVars.emplace_back(needleVar);

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

TEST_F(CountTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars(haystackVars);
  inputVars.emplace_back(needleVar);

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

TEST_F(CountTest, NextInput) {
  haystackSize = 100;
  auto& invariant = generate();

  std::vector<VarViewId> inputVars(haystackVars);
  inputVars.emplace_back(needleVar);

  expectNextInput(inputVars, invariant);
}

TEST_F(CountTest, NotifyCurrentInputChanged) {
  haystackSize = 100;
  auto& invariant = generate();

  std::vector<VarViewId> inputVars(haystackVars);
  inputVars.emplace_back(needleVar);

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(
            ts, varId,
            varId != needleVar ? haystackVarDist(gen) : needleVarDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(CountTest, Commit) {
  haystackSize = 100;

  auto& invariant = generate();

  std::vector<size_t> indices(haystackSize + 1);
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<VarViewId> inputVars(haystackVars);
  inputVars.emplace_back(needleVar);

  std::vector<Int> committedValues(haystackSize + 1, 0);
  for (Int i = 0; i < haystackSize; ++i) {
    committedValues.at(i) = _solver->committedValue(haystackVars.at(i));
  }
  committedValues.back() = _solver->committedValue(needleVar);

  EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < committedValues.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputVars.at(j)),
                committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputVars.at(i),
                        static_cast<Int>(i) < haystackSize
                            ? haystackVarDist(gen)
                            : needleVarDist(gen));
    } while (oldVal == _solver->value(ts, inputVars.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = _solver->value(ts, outputVar);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

    _solver->commitIf(ts, VarId(inputVars.at(i)));
    committedValues.at(i) = _solver->value(ts, inputVars.at(i));
    _solver->commitIf(ts, VarId(outputVar));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
  }
}

RC_GTEST_FIXTURE_PROP(CountTest, rapidcheck, ()) {
  haystackSize = *rc::gen::inRange(1, 100);

  haystackVarLb = *rc::gen::inRange(std::numeric_limits<Int>::min(),
                                    std::numeric_limits<Int>::max() - 200);

  haystackVarUb = *rc::gen::inRange(haystackVarLb + 1, haystackVarLb + 200);

  needleVarLb = *rc::gen::inRange(std::numeric_limits<Int>::min(),
                                  std::numeric_limits<Int>::max() - 100);
  needleVarUb = *rc::gen::inRange(needleVarLb + 1, needleVarLb + 100);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (size_t i = 0; i < haystackVars.size(); ++i) {
        if (randBool()) {
          _solver->setValue(haystackVars.at(i), haystackVarDist(gen));
        }
      }
      if (randBool()) {
        _solver->setValue(needleVar, needleVarDist(gen));
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

class MockCount : public Count {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Count::registerVars();
  }
  explicit MockCount(SolverBase& solver, VarViewId output, VarViewId needleVar,
                     std::vector<VarViewId>&& varArray)
      : Count(solver, output, needleVar, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Count::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Count::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Count::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Count::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Count::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(CountTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const size_t numArgs = 10;
    const VarViewId needleVar = _solver->makeIntVar(0, 0, numArgs);
    std::vector<VarViewId> args;
    for (size_t value = 1; value <= numArgs; ++value) {
      args.push_back(_solver->makeIntVar(static_cast<Int>(value), 1,
                                         static_cast<Int>(numArgs)));
    }
    const VarViewId modifiedVarId = args.front();
    const VarViewId output = _solver->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCount>(
        &_solver->makeInvariant<MockCount>(*_solver, output, needleVar,
                                           std::move(args)),
        {propMode, markingMode, numArgs + 2, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
