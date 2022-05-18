#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/boolLinear.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class BoolLinearTest : public InvariantTest {
 protected:
  const size_t numInputs = 1000;
  Int inputLb = 0;
  Int inputUb = std::numeric_limits<Int>::max();
  Int coeffLb = -10000;
  Int coeffUb = 10000;
  std::vector<VarId> inputs;
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

  Int computeOutput(const Timestamp ts, const std::vector<VarId>& violVars,
                    const std::vector<Int>& coefficients) {
    std::vector<Int> values(violVars.size(), 0);
    for (size_t i = 0; i < violVars.size(); ++i) {
      values.at(i) = engine->value(ts, violVars.at(i));
    }
    return computeOutput(values, coefficients);
  }

  Int computeOutput(const std::vector<Int>& violations,
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
  engine->open();

  for (const Int aCoef : coefVec) {
    for (const Int bCoef : coefVec) {
      for (const Int cCoef : coefVec) {
        std::vector<VarId> vars{engine->makeIntVar(0, 0, 10),
                                engine->makeIntVar(0, 0, 10),
                                engine->makeIntVar(0, 0, 10)};
        const VarId outputId = engine->makeIntVar(0, 0, 2);
        BoolLinear& invariant = engine->makeInvariant<BoolLinear>(
            std::vector<Int>{aCoef, bCoef, cCoef}, std::vector<VarId>(vars),
            outputId);
        for (const auto& [aLb, aUb] : boundVec) {
          EXPECT_TRUE(aLb <= aUb);
          engine->updateBounds(vars.at(0), aLb, aUb, false);
          for (const auto& [bLb, bUb] : boundVec) {
            EXPECT_TRUE(bLb <= bUb);
            engine->updateBounds(vars.at(1), bLb, bUb, false);
            for (const auto& [cLb, cUb] : boundVec) {
              EXPECT_TRUE(cLb <= cUb);
              engine->updateBounds(vars.at(2), cLb, cUb, false);
              invariant.updateBounds(*engine);

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

              ASSERT_EQ(aMin + bMin + cMin, engine->lowerBound(outputId));
              ASSERT_EQ(aMax + bMax + cMax, engine->upperBound(outputId));
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

  engine->open();

  const VarId a = engine->makeIntVar(iDist(gen), iLb, iUb);
  const VarId b = engine->makeIntVar(iDist(gen), iLb, iUb);
  const VarId c = engine->makeIntVar(iDist(gen), iLb, iUb);

  inputs = std::vector<VarId>{a, b, c};
  coeffs = std::vector<Int>{cDist(gen), cDist(gen), cDist(gen)};

  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());

  BoolLinear& invariant = engine->makeInvariant<BoolLinear>(
      std::vector<Int>(coeffs), std::vector<VarId>(inputs), outputId);
  engine->close();

  for (Int aVal = iLb; aVal <= iUb; ++aVal) {
    for (Int bVal = iLb; bVal <= iUb; ++bVal) {
      for (Int cVal = iLb; cVal <= iUb; ++cVal) {
        engine->setValue(engine->currentTimestamp(), a, aVal);
        engine->setValue(engine->currentTimestamp(), b, bVal);
        engine->setValue(engine->currentTimestamp(), c, cVal);
        const Int expectedOutput =
            computeOutput(engine->currentTimestamp(), inputs, coeffs);
        invariant.recompute(engine->currentTimestamp(), *engine);
        EXPECT_EQ(expectedOutput,
                  engine->value(engine->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(BoolLinearTest, NotifyInputChanged) {
  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  BoolLinear& invariant = engine->makeInvariant<BoolLinear>(
      std::vector<Int>(coeffs), std::vector<VarId>(inputs), outputId);
  engine->close();

  for (size_t i = 0; i < inputs.size(); ++i) {
    const Int oldVal = engine->value(engine->currentTimestamp(), inputs.at(i));
    do {
      engine->setValue(engine->currentTimestamp(), inputs.at(i),
                       inputValueDist(gen));
    } while (oldVal == engine->value(engine->currentTimestamp(), inputs.at(i)));

    const Int expectedOutput =
        computeOutput(engine->currentTimestamp(), inputs, coeffs);

    invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                 LocalId(i));
    EXPECT_EQ(expectedOutput,
              engine->value(engine->currentTimestamp(), outputId));
  }
}

TEST_F(BoolLinearTest, NextInput) {
  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }

  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());

  std::shuffle(inputs.begin(), inputs.end(), rng);

  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  BoolLinear& invariant = engine->makeInvariant<BoolLinear>(
      std::vector<Int>(coeffs), std::vector<VarId>(inputs), outputId);

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
      const VarId varId = invariant.nextInput(ts, *engine);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(BoolLinearTest, NotifyCurrentInputChanged) {
  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  BoolLinear& invariant = engine->makeInvariant<BoolLinear>(
      std::vector<Int>(coeffs), std::vector<VarId>(inputs), outputId);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, inputValueDist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts, *engine);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs, coeffs));
    }
  }
}

TEST_F(BoolLinearTest, Commit) {
  std::vector<size_t> indices(numInputs, 0);
  std::vector<Int> committedValues(numInputs, 0);

  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    indices.at(i) = i;
    const Int inputVal = inputValueDist(gen);
    committedValues.at(i) = inputVal;
    coeffs.at(i) = coeffDist(gen);
    inputs.at(i) = engine->makeIntVar(inputVal, inputLb, inputUb);
  }
  std::shuffle(indices.begin(), indices.end(), rng);

  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  BoolLinear& invariant = engine->makeInvariant<BoolLinear>(
      std::vector<Int>(coeffs), std::vector<VarId>(inputs), outputId);
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
            computeOutput(engine->currentTimestamp(), inputs, coeffs));

  for (const size_t i : indices) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(i);
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(engine->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      engine->setValue(ts, inputs.at(i), inputValueDist(gen));
    } while (oldVal == engine->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, *engine, LocalId(i));

    // incremental value
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

static Int clamp(Int val, Int lb, Int ub) {
  return std::max(lb, std::min(ub, val));
}

RC_GTEST_FIXTURE_PROP(BoolLinearTest, ShouldAlwaysBeSum,
                      (Int aCoef, Int aVal, Int bCoef, Int bVal, Int cCoef,
                       Int cVal)) {
  engine->open();
  const Int globalLb = std::numeric_limits<Int>::min() / static_cast<Int>(3);
  const Int globalUb = std::numeric_limits<Int>::max() / static_cast<Int>(3);

  aCoef = clamp(aCoef, globalLb, globalUb);
  bCoef = clamp(bCoef, globalLb, globalUb);
  cCoef = clamp(cCoef, globalLb, globalUb);

  const VarId a = engine->makeIntVar(0, 0, std::numeric_limits<Int>::max());
  const VarId b = engine->makeIntVar(0, 0, std::numeric_limits<Int>::max());
  const VarId c = engine->makeIntVar(0, 0, std::numeric_limits<Int>::max());
  const VarId output =
      engine->makeIntVar(aCoef + bCoef + cCoef, std::numeric_limits<Int>::min(),
                         std::numeric_limits<Int>::max());
  engine->makeInvariant<BoolLinear>(std::vector<Int>{aCoef, bCoef, cCoef},
                                    std::vector<VarId>{a, b, c}, output);
  engine->close();

  aVal = std::max<Int>(0, aVal);
  bVal = std::max<Int>(0, bVal);
  cVal = std::max<Int>(0, cVal);

  engine->beginMove();
  engine->setValue(a, aVal);
  engine->setValue(b, bVal);
  engine->setValue(c, cVal);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  const Int expected = aCoef * static_cast<Int>(aVal == 0) +
                       bCoef * static_cast<Int>(bVal == 0) +
                       cCoef * static_cast<Int>(cVal == 0);
  const Int actual = engine->committedValue(output);
  if (expected != actual) {
    RC_ASSERT(expected == actual);
  }
}

class MockBoolLinear : public BoolLinear {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    BoolLinear::registerVars(engine);
  }
  MockBoolLinear(std::vector<VarId> X, VarId b) : BoolLinear(X, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return BoolLinear::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return BoolLinear::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          BoolLinear::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          BoolLinear::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      BoolLinear::commit(t, engine);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));
  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp t, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
};
TEST_F(BoolLinearTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    const Int numArgs = 10;
    for (Int value = 1; value <= numArgs; ++value) {
      args.push_back(engine->makeIntVar(value, 1, numArgs));
    }
    const VarId modifiedVarId = args.front();
    const VarId output = engine->makeIntVar(-10, -100, numArgs * numArgs);
    testNotifications<MockBoolLinear>(
        &engine->makeInvariant<MockBoolLinear>(args, output), propMode,
        markingMode, numArgs + 1, modifiedVarId, 5, output);
  }
}

}  // namespace
