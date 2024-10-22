#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolLinearTest : public InvariantTest {
 protected:
  const size_t numInputs = 1000;
  Int inputLb = 0;
  Int inputUb = std::numeric_limits<Int>::max();
  Int coeffLb = -10000;
  Int coeffUb = 10000;
  std::vector<VarViewId> inputs;
  std::vector<Int> coeffs;
  std::uniform_int_distribution<Int> inputValueDist;
  std::uniform_int_distribution<Int> coeffDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();

    inputs.resize(numInputs, NULL_ID);
    coeffs.resize(numInputs, 0);
    std::vector<Int> bounds{0,
                            (inputUb / static_cast<Int>(numInputs)) / coeffLb,
                            (inputUb / static_cast<Int>(numInputs)) / coeffUb};
    const auto [lb, ub] = std::minmax_element(bounds.begin(), bounds.end());
    inputLb = std::max<Int>(0, *lb);
    inputUb = *ub;
    inputValueDist = std::uniform_int_distribution<Int>(inputLb, inputUb);
    coeffDist = std::uniform_int_distribution<Int>(coeffLb, coeffUb);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputs.clear();
  }

  Int computeOutput(const Timestamp ts, const std::vector<VarViewId>& violVars,
                    const std::vector<Int>& coefficients) {
    std::vector<Int> values(violVars.size(), 0);
    for (size_t i = 0; i < violVars.size(); ++i) {
      values.at(i) = _solver->value(ts, violVars.at(i));
    }
    return computeOutput(values, coefficients);
  }

  static Int computeOutput(const std::vector<Int>& violations,
                           const std::vector<Int>& coefficients) {
    Int sum = 0;
    for (size_t i = 0; i < violations.size(); ++i) {
      sum += static_cast<Int>(violations.at(i) == 0) * coefficients.at(i);
    }
    return sum;
  }
};

TEST_F(BoolLinearTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {0, 0}, {0, 1}, {0, 100}, {150, 250}};
  std::vector<Int> coefVec{-1000, -1, 0, 1, 1000};
  _solver->open();

  for (const Int aCoef : coefVec) {
    for (const Int bCoef : coefVec) {
      for (const Int cCoef : coefVec) {
        std::vector<VarViewId> vars{_solver->makeIntVar(0, 0, 10),
                                    _solver->makeIntVar(0, 0, 10),
                                    _solver->makeIntVar(0, 0, 10)};
        const VarViewId outputId = _solver->makeIntVar(0, 0, 2);
        BoolLinear& invariant = _solver->makeInvariant<BoolLinear>(
            *_solver, outputId, std::vector<Int>{aCoef, bCoef, cCoef},
            std::vector<VarViewId>(vars));
        for (const auto& [aLb, aUb] : boundVec) {
          EXPECT_TRUE(aLb <= aUb);
          _solver->updateBounds(VarId(vars.at(0)), aLb, aUb, false);
          for (const auto& [bLb, bUb] : boundVec) {
            EXPECT_TRUE(bLb <= bUb);
            _solver->updateBounds(VarId(vars.at(1)), bLb, bUb, false);
            for (const auto& [cLb, cUb] : boundVec) {
              EXPECT_TRUE(cLb <= cUb);
              _solver->updateBounds(VarId(vars.at(2)), cLb, cUb, false);
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

              ASSERT_EQ(aMin + bMin + cMin, _solver->lowerBound(outputId));
              ASSERT_EQ(aMax + bMax + cMax, _solver->upperBound(outputId));
            }
          }
        }
      }
    }
  }
}

TEST_F(BoolLinearTest, Recompute) {
  const Int iLb = 0;
  const Int iUb = 10;
  const Int cLb = -10;
  const Int cUb = 10;

  ASSERT_TRUE(iLb <= iUb);
  ASSERT_TRUE(cLb <= cUb);

  std::uniform_int_distribution<Int> iDist(iLb, iUb);
  std::uniform_int_distribution<Int> cDist(cLb, cUb);

  _solver->open();

  const VarViewId a = _solver->makeIntVar(iDist(gen), iLb, iUb);
  const VarViewId b = _solver->makeIntVar(iDist(gen), iLb, iUb);
  const VarViewId c = _solver->makeIntVar(iDist(gen), iLb, iUb);

  inputs = std::vector<VarViewId>{a, b, c};
  coeffs = std::vector<Int>{cDist(gen), cDist(gen), cDist(gen)};

  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());

  BoolLinear& invariant = _solver->makeInvariant<BoolLinear>(
      *_solver, outputId, std::vector<Int>(coeffs),
      std::vector<VarViewId>(inputs));
  _solver->close();

  for (Int aVal = iLb; aVal <= iUb; ++aVal) {
    for (Int bVal = iLb; bVal <= iUb; ++bVal) {
      for (Int cVal = iLb; cVal <= iUb; ++cVal) {
        _solver->setValue(_solver->currentTimestamp(), a, aVal);
        _solver->setValue(_solver->currentTimestamp(), b, bVal);
        _solver->setValue(_solver->currentTimestamp(), c, cVal);
        const Int expectedOutput =
            computeOutput(_solver->currentTimestamp(), inputs, coeffs);
        invariant.recompute(_solver->currentTimestamp());
        EXPECT_EQ(expectedOutput,
                  _solver->value(_solver->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(BoolLinearTest, NotifyInputChanged) {
  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }
  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  BoolLinear& invariant = _solver->makeInvariant<BoolLinear>(
      *_solver, outputId, std::vector<Int>(coeffs),
      std::vector<VarViewId>(inputs));
  _solver->close();

  Timestamp ts = _solver->currentTimestamp() + 1;

  for (size_t i = 0; i < inputs.size(); ++i) {
    const Int oldVal = _solver->value(ts, VarId(inputs.at(i)));
    do {
      _solver->setValue(ts, inputs.at(i), inputValueDist(gen));
    } while (oldVal == _solver->value(ts, inputs.at(i)));

    const Int expectedOutput = computeOutput(ts, inputs, coeffs);

    invariant.notifyInputChanged(ts, LocalId(i));
    EXPECT_EQ(expectedOutput, _solver->value(ts, outputId));
  }
}

TEST_F(BoolLinearTest, NextInput) {
  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }

  const VarViewId minVarId =
      *std::min_element(inputs.begin(), inputs.end(),
                        [&](const VarViewId& a, const VarViewId& b) {
                          return size_t(a) < size_t(b);
                        });
  const VarViewId maxVarId =
      *std::max_element(inputs.begin(), inputs.end(),
                        [&](const VarViewId& a, const VarViewId& b) {
                          return size_t(a) < size_t(b);
                        });

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  BoolLinear& invariant = _solver->makeInvariant<BoolLinear>(
      *_solver, outputId, std::vector<Int>(coeffs),
      std::vector<VarViewId>(inputs));

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(size_t(maxVarId) + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
      const VarViewId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_LE(size_t(minVarId), size_t(varId));
      EXPECT_GE(size_t(maxVarId), size_t(varId));
      EXPECT_FALSE(notified.at(size_t(varId)));
      notified.at(size_t(varId)) = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t i = size_t(minVarId); i <= size_t(maxVarId); ++i) {
      EXPECT_TRUE(notified.at(i));
    }
  }
}

TEST_F(BoolLinearTest, NotifyCurrentInputChanged) {
  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = _solver->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }
  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  BoolLinear& invariant = _solver->makeInvariant<BoolLinear>(
      *_solver, outputId, std::vector<Int>(coeffs),
      std::vector<VarViewId>(inputs));
  _solver->close();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, inputValueDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputId),
                computeOutput(ts, inputs, coeffs));
    }
  }
}

TEST_F(BoolLinearTest, Commit) {
  std::vector<size_t> indices(numInputs, 0);
  std::vector<Int> committedValues(numInputs, 0);

  _solver->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    const Int inputVal = inputValueDist(gen);
    committedValues.at(i) = inputVal;
    coeffs.at(i) = coeffDist(gen);
    inputs.at(i) = _solver->makeIntVar(inputVal, inputLb, inputUb);
  }
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarViewId outputId = _solver->makeIntVar(
      0, std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  BoolLinear& invariant = _solver->makeInvariant<BoolLinear>(
      *_solver, outputId, std::vector<Int>(coeffs),
      std::vector<VarViewId>(inputs));
  _solver->close();

  EXPECT_EQ(_solver->value(_solver->currentTimestamp(), outputId),
            computeOutput(_solver->currentTimestamp(), inputs, coeffs));

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputs.at(i), inputValueDist(gen));
    } while (oldVal == _solver->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = _solver->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, _solver->value(ts, outputId));

    _solver->commitIf(ts, VarId(inputs.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputs.at(i)));
    _solver->commitIf(ts, VarId(outputId));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputId));
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
