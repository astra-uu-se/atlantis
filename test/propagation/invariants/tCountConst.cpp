#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class CountConstTest : public InvariantTest {
 public:
  Int numInputVars{3};
  std::vector<VarViewId> inputVars;
  VarViewId outputVar{NULL_ID};
  Int needleVal{10};
  Int inputVarLb{-10};
  Int inputVarUb{10};
  std::uniform_int_distribution<Int> inputVarDist;

  CountConst& generate(bool generateInputVars = true) {
    inputVars.clear();
    inputVars.reserve(numInputVars);
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    if (generateInputVars) {
      for (Int i = 0; i < numInputVars; ++i) {
        inputVars.emplace_back(
            makeIntVar(inputVarLb, inputVarUb, inputVarDist));
      }
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    CountConst& invariant = _solver->makeInvariant<CountConst>(
        *_solver, outputVar, needleVal, std::vector<VarViewId>(inputVars));

    _solver->close();
    return invariant;
  }

  Int computeOutput(Timestamp ts) {
    std::vector<Int> values(inputVars.size(), 0);
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = _solver->value(ts, inputVars.at(i));
    }
    return computeOutput(values);
  }

  Int computeOutput(bool committedValue = false) {
    std::vector<Int> values(inputVars.size(), 0);
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = committedValue ? _solver->committedValue(inputVars.at(i))
                                    : _solver->currentValue(inputVars.at(i));
    }
    return computeOutput(values);
  }

  Int computeOutput(const std::vector<Int>& values) {
    Int occurrences = 0;
    for (const Int val : values) {
      occurrences += (val == needleVal ? 1 : 0);
    }
    return occurrences;
  }
};

TEST_F(CountConstTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-10, 0}, {-5, 5}, {0, 10}, {15, 20}};
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

        ASSERT_GE(0, _solver->lowerBound(outputVar));
        ASSERT_LE(inputVars.size(), _solver->upperBound(outputVar));
      }
    }
  }
}

TEST_F(CountConstTest, Recompute) {
  generateState = GenerateState::LB;

  numInputVars = 3;
  const Int lb = -5;
  const Int ub = 5;

  std::uniform_int_distribution<Int> dist(lb, ub);

  for (Int needleVal = lb; needleVal <= ub; ++needleVal) {
    _solver->open();

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

TEST_F(CountConstTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  numInputVars = 3;
  const Int lb = -10;
  const Int ub = 10;

  for (Int needleVal = lb; needleVal <= ub; ++needleVal) {
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

TEST_F(CountConstTest, NextInput) {
  numInputVars = 100;
  inputVarLb = -10;
  inputVarUb = 10;
  needleVal = 0;

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(CountConstTest, NotifyCurrentInputChanged) {
  numInputVars = 100;
  const Int lb = -10;
  const Int ub = 10;

  Timestamp ts = _solver->currentTimestamp() + ub - lb + 2;

  for (Int needleVal = lb; needleVal <= ub; ++needleVal) {
    auto& invariant = generate();

    for (Int i = 0; i < 4; ++i) {
      ++ts;

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
}

TEST_F(CountConstTest, Commit) {
  numInputVars = 100;
  const Int lb = -10;
  const Int ub = 10;

  std::vector<size_t> indices(numInputVars, 0);
  std::iota(indices.begin(), indices.end(), 0);

  std::vector<Int> committedValues(numInputVars);

  for (Int needleVal = lb; needleVal <= ub; ++needleVal) {
    auto& invariant = generate();

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
}

RC_GTEST_FIXTURE_PROP(CountConstTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);

  inputVarLb = *rc::gen::inRange(std::numeric_limits<Int>::min(),
                                 std::numeric_limits<Int>::max() - 200);

  inputVarUb = *rc::gen::inRange(inputVarLb + 1, inputVarUb + 200);

  needleVal = *rc::gen::arbitrary<Int>();

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

class MockCountConst : public CountConst {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    CountConst::registerVars();
  }
  explicit MockCountConst(SolverBase& solver, VarViewId output, Int needleVal,
                          std::vector<VarViewId>&& varArray)
      : CountConst(solver, output, needleVal, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return CountConst::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return CountConst::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          CountConst::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          CountConst::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      CountConst::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(CountConstTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const size_t numArgs = 10;
    const Int needleVal = 5;
    std::vector<VarViewId> varArray;
    for (size_t value = 1; value <= numArgs; ++value) {
      varArray.push_back(_solver->makeIntVar(static_cast<Int>(value), 1,
                                             static_cast<Int>(numArgs)));
    }
    const VarViewId modifiedVarId = varArray.front();
    const VarViewId output = _solver->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockCountConst>(
        &_solver->makeInvariant<MockCountConst>(*_solver, output, needleVal,
                                                std::move(varArray)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
