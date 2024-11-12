#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/linear.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class LinearTest : public InvariantTest {
 protected:
  Int numInputVars{100};
  Int inputVarLb{std::numeric_limits<Int>::min()};
  Int inputVarUb{std::numeric_limits<Int>::max()};
  Int coeffLb{-100};
  Int coeffUb{100};

  std::vector<VarViewId> inputVars;
  std::vector<Int> coeffs;
  VarViewId outputVar{NULL_ID};
  std::uniform_int_distribution<Int> inputVarDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    numInputVars = 100;
    coeffLb = -100;
    coeffUb = 100;
  }

  Linear& generate() {
    if (!_solver->isOpen()) {
      _solver->open();
    }

    Int cLb =
        (!coeffs.empty() && coeffs.size() < static_cast<size_t>(numInputVars))
            ? coeffs.front()
            : coeffLb;
    Int cUb =
        (!coeffs.empty() && coeffs.size() < static_cast<size_t>(numInputVars))
            ? coeffs.front()
            : coeffUb;

    for (size_t i = 0; i < coeffs.size(); ++i) {
      cLb = std::min(cLb, coeffs.at(i));
      cUb = std::max(cUb, coeffs.at(i));
    }

    if (coeffs.size() < static_cast<size_t>(numInputVars)) {
      auto coeffDist = std::uniform_int_distribution<Int>(coeffLb, coeffUb);
      coeffs.reserve(numInputVars);
      for (Int i = static_cast<Int>(coeffs.size()); i < numInputVars; ++i) {
        coeffs.emplace_back(coeffDist(gen));
      }
    }

    std::vector<Int> bounds{
        (std::numeric_limits<Int>::min() / static_cast<Int>(numInputVars)) /
            coeffLb,
        (std::numeric_limits<Int>::min() / static_cast<Int>(numInputVars)) /
            coeffUb,
        (std::numeric_limits<Int>::max() / static_cast<Int>(numInputVars)) /
            coeffLb,
        (std::numeric_limits<Int>::max() / static_cast<Int>(numInputVars)) /
            coeffUb};
    const auto [lb, ub] = std::minmax_element(bounds.begin(), bounds.end());
    inputVarLb = std::max(inputVarLb, *lb);
    inputVarUb = std::min(inputVarUb, *ub);
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);

    inputVars.clear();
    inputVars.reserve(numInputVars);
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(makeIntVar(inputVarLb, inputVarUb, inputVarDist));
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    auto& invariant = _solver->makeInvariant<Linear>(
        *_solver, outputVar, std::vector<Int>(coeffs),
        std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputVars.clear();
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
    Int sum = 0;
    for (size_t i = 0; i < values.size(); ++i) {
      sum += values.at(i) * coeffs.at(i);
    }
    return sum;
  }
};

TEST_F(LinearTest, UpdateBounds) {
  numInputVars = 3;

  std::vector<std::pair<Int, Int>> boundVec{
      {-250, -150}, {-100, 0}, {-50, 50}, {0, 100}, {150, 250}};
  std::vector<Int> coefVec{-1000, -1, 0, 1, 1000};

  inputVarLb = boundVec.front().first;
  inputVarUb = boundVec.back().second;

  coeffs = std::vector<Int>(numInputVars, coefVec.front());

  for (const Int aCoef : coefVec) {
    coeffs.at(0) = aCoef;
    for (const Int bCoef : coefVec) {
      coeffs.at(1) = bCoef;
      for (const Int cCoef : coefVec) {
        coeffs.at(2) = cCoef;
        auto invariant = generate();
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

              const Int aMin = std::min(aLb * aCoef, aUb * aCoef);
              const Int aMax = std::max(aLb * aCoef, aUb * aCoef);
              const Int bMin = std::min(bLb * bCoef, bUb * bCoef);
              const Int bMax = std::max(bLb * bCoef, bUb * bCoef);
              const Int cMin = std::min(cLb * cCoef, cUb * cCoef);
              const Int cMax = std::max(cLb * cCoef, cUb * cCoef);

              ASSERT_EQ(aMin + bMin + cMin, _solver->lowerBound(outputVar));
              ASSERT_EQ(aMax + bMax + cMax, _solver->upperBound(outputVar));
            }
          }
        }
      }
    }
  }
}

TEST_F(LinearTest, Recompute) {
  generateState = GenerateState::LB;

  numInputVars = 3;
  inputVarLb = -5;
  inputVarUb = -5;
  coeffs = std::vector<Int>{-10000, 1, 10000};

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

TEST_F(LinearTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  numInputVars = 3;
  inputVarLb = -5;
  inputVarUb = -5;
  coeffs = std::vector<Int>{-10000, 1, 10000};

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

TEST_F(LinearTest, NextInput) {
  auto& invariant = generate();
  expectNextInput(inputVars, invariant);
}

TEST_F(LinearTest, NotifyCurrentInputChanged) {
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

TEST_F(LinearTest, Commit) {
  auto& invariant = generate();

  std::vector<Int> committedValues(numInputVars, 0);
  for (Int i = 0; i < numInputVars; ++i) {
    committedValues.at(i) = _solver->committedValue(inputVars.at(i));
  }
  std::vector<size_t> indices(numInputVars, 0);
  std::iota(indices.begin(), indices.end(), 0);

  EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

  Timestamp ts = _solver->currentTimestamp();
  for (const size_t i : indices) {
    ts += 2;
    for (Int j = 0; j < numInputVars; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputVars.at(j)),
                committedValues.at(j));
      ASSERT_EQ(_solver->value(ts, inputVars.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    Int newVal;
    do {
      newVal = inputVarDist(gen);
    } while (newVal == oldVal);

    _solver->setValue(ts, inputVars.at(i), newVal);

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

RC_GTEST_FIXTURE_PROP(LinearTest, rapidcheck, ()) {
  _solver->open();
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

class MockLinear : public Linear {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Linear::registerVars();
  }
  explicit MockLinear(SolverBase& solver, VarViewId output,
                      std::vector<VarViewId>&& varArray)
      : Linear(solver, output, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Linear::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Linear::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Linear::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Linear::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Linear::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(LinearTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<VarViewId> args;
    const size_t numArgs = 10;
    for (size_t value = 1; value <= numArgs; ++value) {
      args.push_back(_solver->makeIntVar(static_cast<Int>(value), 1,
                                         static_cast<Int>(numArgs)));
    }
    const VarViewId modifiedVarId = args.front();
    const VarViewId output =
        _solver->makeIntVar(-10, -100, static_cast<Int>(numArgs * numArgs));
    testNotifications<MockLinear>(
        &_solver->makeInvariant<MockLinear>(*_solver, output, std::move(args)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
