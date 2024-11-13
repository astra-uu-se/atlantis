#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolLinearTest : public InvariantTest {
 public:
  Int numInputVars{3};
  Int inputVarLb{0};
  Int inputVarUb{2};
  std::vector<VarViewId> inputVars;
  std::vector<Int> coeffs;
  std::uniform_int_distribution<Int> inputVarDist;
  VarViewId outputVar{NULL_ID};

  void SetUp() override {
    InvariantTest::SetUp();

    inputVars.clear();
    coeffs.resize(numInputVars, 1);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputVars.clear();
  }

  BoolLinear& generate() {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);

    const Int cs = static_cast<Int>(coeffs.size());

    if (cs < numInputVars) {
      Int coeffLb =
          std::numeric_limits<Int>::min() / static_cast<Int>(numInputVars + 1);
      Int coeffUb =
          std::numeric_limits<Int>::max() / static_cast<Int>(numInputVars + 1);
      auto coeffDist = std::uniform_int_distribution<Int>(coeffLb, coeffUb);
      coeffs.reserve(numInputVars);
      for (Int i = 0; i < numInputVars; ++i) {
        if (i < cs) {
          if (coeffs.at(i) < coeffLb || coeffUb < coeffs.at(i)) {
            coeffs.at(i) = coeffDist(gen);
          }
        } else {
          coeffs.emplace_back(coeffDist(gen));
        }
      }
    }

    inputVars.clear();
    inputVars.reserve(numInputVars);
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(makeIntVar(inputVarLb, inputVarUb, inputVarDist));
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    auto& invariant = _solver->makeInvariant<BoolLinear>(
        *_solver, outputVar, std::vector<Int>(coeffs),
        std::vector<VarViewId>(inputVars));
    _solver->close();
    return invariant;
  }

  Int computeOutput(Timestamp ts) {
    std::vector<Int> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = _solver->value(ts, inputVars.at(i));
    }
    return computeOutput(values);
  }

  Int computeOutput(bool committedValue = false) {
    std::vector<Int> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = committedValue ? _solver->committedValue(inputVars.at(i))
                                    : _solver->currentValue(inputVars.at(i));
    }
    return computeOutput(values);
  }

  Int computeOutput(const std::vector<Int>& violations) {
    Int sum = 0;
    for (size_t i = 0; i < violations.size(); ++i) {
      sum += static_cast<Int>(violations.at(i) == 0) * coeffs.at(i);
    }
    return sum;
  }
};

TEST_F(BoolLinearTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 100}, {150, 250}};
  std::vector<Int> coefVec{-1000, -1, 0, 1, 1000};
  numInputVars = 3;
  coeffs = std::vector<Int>(numInputVars, coefVec.front());

  for (const Int aCoef : coefVec) {
    coeffs.at(0) = aCoef;
    for (const Int bCoef : coefVec) {
      coeffs.at(1) = bCoef;
      for (const Int cCoef : coefVec) {
        coeffs.at(2) = cCoef;
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

              const Int aMin = std::min(static_cast<Int>(aLb == 0) * aCoef,
                                        static_cast<Int>(aUb == 0) * aCoef);
              const Int aMax = std::max(static_cast<Int>(aLb == 0) * aCoef,
                                        static_cast<Int>(aUb == 0) * aCoef);
              const Int bMin = std::min(static_cast<Int>(bLb == 0) * bCoef,
                                        static_cast<Int>(bUb == 0) * bCoef);
              const Int bMax = std::max(static_cast<Int>(bLb == 0) * bCoef,
                                        static_cast<Int>(bUb == 0) * bCoef);
              const Int cMin = std::min(static_cast<Int>(cLb == 0) * cCoef,
                                        static_cast<Int>(cUb == 0) * cCoef);
              const Int cMax = std::max(static_cast<Int>(cLb == 0) * cCoef,
                                        static_cast<Int>(cUb == 0) * cCoef);

              ASSERT_EQ(aMin + bMin + cMin, _solver->lowerBound(outputVar));
              ASSERT_EQ(aMax + bMax + cMax, _solver->upperBound(outputVar));
            }
          }
        }
      }
    }
  }
}

TEST_F(BoolLinearTest, Recompute) {
  generateState = GenerateState::LB;

  numInputVars = 4;

  coeffs = std::vector<Int>(numInputVars);
  std::iota(coeffs.begin(), coeffs.end(), -(numInputVars / 2));

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

TEST_F(BoolLinearTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  coeffs = std::vector<Int>(numInputVars);
  std::iota(coeffs.begin(), coeffs.end(), -(numInputVars / 2));
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

TEST_F(BoolLinearTest, NextInput) {
  coeffs.clear();
  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(BoolLinearTest, NotifyCurrentInputChanged) {
  coeffs = std::vector<Int>(numInputVars);
  std::iota(coeffs.begin(), coeffs.end(), -(numInputVars / 2));
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

TEST_F(BoolLinearTest, Commit) {
  coeffs = std::vector<Int>(numInputVars);
  std::iota(coeffs.begin(), coeffs.end(), -(numInputVars / 2));
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

static Int clamp(Int val, Int lb, Int ub) {
  return std::max(lb, std::min(ub, val));
}

RC_GTEST_FIXTURE_PROP(BoolLinearTest, ShouldAlwaysBeSum,
                      (Int aCoef, Int aVal, Int bCoef, Int bVal, Int cCoef,
                       Int cVal)) {
  _solver->open();
  const Int globalLb = std::numeric_limits<Int>::min() / static_cast<Int>(3);
  const Int globalUb = std::numeric_limits<Int>::max() / static_cast<Int>(3);

  aCoef = clamp(aCoef, globalLb, globalUb);
  bCoef = clamp(bCoef, globalLb, globalUb);
  cCoef = clamp(cCoef, globalLb, globalUb);

  const VarViewId a =
      _solver->makeIntVar(0, 0, std::numeric_limits<Int>::max());
  const VarViewId b =
      _solver->makeIntVar(0, 0, std::numeric_limits<Int>::max());
  const VarViewId c =
      _solver->makeIntVar(0, 0, std::numeric_limits<Int>::max());
  const VarViewId output = _solver->makeIntVar(aCoef + bCoef + cCoef,
                                               std::numeric_limits<Int>::min(),
                                               std::numeric_limits<Int>::max());
  _solver->makeInvariant<BoolLinear>(*_solver, output,
                                     std::vector<Int>{aCoef, bCoef, cCoef},
                                     std::vector<VarViewId>{a, b, c});
  _solver->close();

  aVal = std::max<Int>(0, aVal);
  bVal = std::max<Int>(0, bVal);
  cVal = std::max<Int>(0, cVal);

  _solver->beginMove();
  _solver->setValue(a, aVal);
  _solver->setValue(b, bVal);
  _solver->setValue(c, cVal);
  _solver->endMove();

  _solver->beginCommit();
  _solver->query(output);
  _solver->endCommit();

  const Int expected = aCoef * static_cast<Int>(aVal == 0) +
                       bCoef * static_cast<Int>(bVal == 0) +
                       cCoef * static_cast<Int>(cVal == 0);
  const Int actual = _solver->committedValue(output);
  if (expected != actual) {
    RC_ASSERT(expected == actual);
  }
}

RC_GTEST_FIXTURE_PROP(BoolLinearTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange<Int>(1, 5);

  generate();

  std::vector<std::uniform_int_distribution<Int>> valDists;
  valDists.reserve(numInputVars);

  for (Int i = 0; i < numInputVars; ++i) {
    const Int v1 = *rc::gen::inRange<Int>(0, 1);
    const Int v2 = *rc::gen::inRange<Int>(0, 1);
    valDists.emplace_back(std::min(v1, v2), std::max(v1, v2));
  }

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (size_t i = 0; i < inputVars.size(); ++i) {
        if (randBool()) {
          _solver->setValue(inputVars.at(i), valDists.at(i)(gen));
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

class MockBoolLinear : public BoolLinear {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolLinear::registerVars();
  }
  explicit MockBoolLinear(SolverBase& solver, VarViewId output,
                          std::vector<VarViewId>&& varArray)
      : BoolLinear(solver, output, std::move(varArray)) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolLinear::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolLinear::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolLinear::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          BoolLinear::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolLinear::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BoolLinearTest, SolverIntegration) {
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
    testNotifications<MockBoolLinear>(
        &_solver->makeInvariant<MockBoolLinear>(*_solver, output,
                                                std::move(args)),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace atlantis::testing
