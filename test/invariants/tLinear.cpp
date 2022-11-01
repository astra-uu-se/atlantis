#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/linear.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class LinearTest : public InvariantTest {
 protected:
  const size_t numInputs = 1000;
  Int inputLb = std::numeric_limits<Int>::min();
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
    std::vector<Int> bounds{(inputLb / static_cast<Int>(numInputs)) / coeffLb,
                            (inputLb / static_cast<Int>(numInputs)) / coeffUb,
                            (inputUb / static_cast<Int>(numInputs)) / coeffLb,
                            (inputUb / static_cast<Int>(numInputs)) / coeffUb};
    const auto [lb, ub] = std::minmax_element(bounds.begin(), bounds.end());
    inputLb = *lb;
    inputUb = *ub;
    inputValueDist = std::uniform_int_distribution<Int>(inputLb, inputUb);
    coeffDist = std::uniform_int_distribution<Int>(coeffLb, coeffUb);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputs.clear();
  }

  Int computeOutput(const Timestamp ts, const std::vector<VarId>& variables,
                    const std::vector<Int>& coefficients) {
    std::vector<Int> values(variables.size(), 0);
    for (size_t i = 0; i < variables.size(); ++i) {
      values.at(i) = engine->value(ts, variables.at(i));
    }
    return computeOutput(values, coefficients);
  }

  Int computeOutput(const std::vector<Int>& values,
                    const std::vector<Int>& coefficients) {
    Int sum = 0;
    for (size_t i = 0; i < values.size(); ++i) {
      sum += values.at(i) * coefficients.at(i);
    }
    return sum;
  }
};

TEST_F(LinearTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-250, -150}, {-100, 0}, {-50, 50}, {0, 100}, {150, 250}};
  std::vector<Int> coefVec{-1000, -1, 0, 1, 1000};
  engine->open();

  for (const Int aCoef : coefVec) {
    for (const Int bCoef : coefVec) {
      for (const Int cCoef : coefVec) {
        std::vector<VarId> vars{engine->makeIntVar(0, 0, 10),
                                engine->makeIntVar(0, 0, 10),
                                engine->makeIntVar(0, 0, 10)};
        const VarId outputId = engine->makeIntVar(0, 0, 2);
        Linear& invariant = engine->makeInvariant<Linear>(
            *engine, outputId, std::vector<Int>{aCoef, bCoef, cCoef},
            std::vector<VarId>(vars));
        for (const auto& [aLb, aUb] : boundVec) {
          EXPECT_TRUE(aLb <= aUb);
          engine->updateBounds(vars.at(0), aLb, aUb, false);
          for (const auto& [bLb, bUb] : boundVec) {
            EXPECT_TRUE(bLb <= bUb);
            engine->updateBounds(vars.at(1), bLb, bUb, false);
            for (const auto& [cLb, cUb] : boundVec) {
              EXPECT_TRUE(cLb <= cUb);
              engine->updateBounds(vars.at(2), cLb, cUb, false);
              invariant.updateBounds();

              const Int aMin = std::min(aLb * aCoef, aUb * aCoef);
              const Int aMax = std::max(aLb * aCoef, aUb * aCoef);
              const Int bMin = std::min(bLb * bCoef, bUb * bCoef);
              const Int bMax = std::max(bLb * bCoef, bUb * bCoef);
              const Int cMin = std::min(cLb * cCoef, cUb * cCoef);
              const Int cMax = std::max(cLb * cCoef, cUb * cCoef);

              ASSERT_EQ(aMin + bMin + cMin, engine->lowerBound(outputId));
              ASSERT_EQ(aMax + bMax + cMax, engine->upperBound(outputId));
            }
          }
        }
      }
    }
  }
}

TEST_F(LinearTest, Recompute) {
  const Int iLb = -10;
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

  Linear& invariant =
      engine->makeInvariant<Linear>(*engine, outputId, coeffs, inputs);
  engine->close();

  for (Int aVal = iLb; aVal <= iUb; ++aVal) {
    for (Int bVal = iLb; bVal <= iUb; ++bVal) {
      for (Int cVal = iLb; cVal <= iUb; ++cVal) {
        engine->setValue(engine->currentTimestamp(), a, aVal);
        engine->setValue(engine->currentTimestamp(), b, bVal);
        engine->setValue(engine->currentTimestamp(), c, cVal);
        const Int expectedOutput =
            computeOutput(engine->currentTimestamp(), inputs, coeffs);
        invariant.recompute(engine->currentTimestamp());
        EXPECT_EQ(expectedOutput,
                  engine->value(engine->currentTimestamp(), outputId));
      }
    }
  }
}

TEST_F(LinearTest, NotifyInputChanged) {
  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Linear& invariant =
      engine->makeInvariant<Linear>(*engine, outputId, coeffs, inputs);
  engine->close();

  const Timestamp ts = engine->currentTimestamp() + 1;

  for (size_t i = 0; i < inputs.size(); ++i) {
    const Int oldVal = engine->value(ts, inputs.at(i));
    do {
      engine->setValue(ts, inputs.at(i), inputValueDist(gen));
    } while (oldVal == engine->value(ts, inputs.at(i)));

    const Int expectedOutput = computeOutput(ts, inputs, coeffs);

    invariant.notifyInputChanged(ts, LocalId(i));
    EXPECT_EQ(expectedOutput, engine->value(ts, outputId));
  }
}

TEST_F(LinearTest, NextInput) {
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
  Linear& invariant =
      engine->makeInvariant<Linear>(*engine, outputId, coeffs, inputs);

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < numInputs; ++i) {
      const VarId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(LinearTest, NotifyCurrentInputChanged) {
  engine->open();
  for (size_t i = 0; i < numInputs; ++i) {
    inputs.at(i) = engine->makeIntVar(inputValueDist(gen), inputLb, inputUb);
    coeffs.at(i) = coeffDist(gen);
  }
  const VarId outputId = engine->makeIntVar(0, std::numeric_limits<Int>::min(),
                                            std::numeric_limits<Int>::max());
  Linear& invariant =
      engine->makeInvariant<Linear>(*engine, outputId, coeffs, inputs);
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, inputValueDist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs, coeffs));
    }
  }
}

TEST_F(LinearTest, Commit) {
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
  Linear& invariant =
      engine->makeInvariant<Linear>(*engine, outputId, coeffs, inputs);
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
            computeOutput(engine->currentTimestamp(), inputs, coeffs));

  Timestamp ts = engine->currentTimestamp();
  for (const size_t i : indices) {
    ts += 2;
    for (size_t j = 0; j < numInputs; ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(engine->committedValue(inputs.at(j)), committedValues.at(j));
      ASSERT_EQ(engine->value(ts, inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    Int newVal;
    do {
      newVal = inputValueDist(gen);
    } while (newVal == oldVal);

    engine->setValue(ts, inputs.at(i), newVal);

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

RC_GTEST_FIXTURE_PROP(LinearTest, ShouldAlwaysBeSum,
                      (Int aCoef, Int aVal, Int bCoef, Int bVal, Int cCoef,
                       Int cVal)) {
  engine->open();
  std::vector<Int> coefficients{aCoef, bCoef, cCoef};
  std::vector<std::pair<Int, Int>> bounds;

  for (const Int co : coefficients) {
    if (co == 0) {
      bounds.emplace_back(std::pair<Int, Int>(0, 0));
    } else {
      if (co == -1 || co == 1) {
        bounds.emplace_back(
            std::pair<Int, Int>(std::numeric_limits<Int>::min() / 3,
                                std::numeric_limits<Int>::max() / 3));
      } else {
        bounds.emplace_back(
            std::pair<Int, Int>((std::numeric_limits<Int>::min() / co) / 3,
                                (std::numeric_limits<Int>::min() / co) / 3));
      }
    }
  }

  const Int aLb = std::min(bounds.at(0).first, bounds.at(0).second);
  const Int aUb = std::max(bounds.at(0).first, bounds.at(0).second);
  const Int bLb = std::min(bounds.at(1).first, bounds.at(1).second);
  const Int bUb = std::max(bounds.at(1).first, bounds.at(1).second);
  const Int cLb = std::min(bounds.at(2).first, bounds.at(2).second);
  const Int cUb = std::max(bounds.at(2).first, bounds.at(2).second);

  const VarId a = engine->makeIntVar(aLb, aLb, aUb);
  const VarId b = engine->makeIntVar(bLb, bLb, bUb);
  const VarId c = engine->makeIntVar(cLb, cLb, cUb);
  const VarId output = engine->makeIntVar(
      aLb * aCoef + bLb * bCoef + cLb * cCoef, std::numeric_limits<Int>::min(),
      std::numeric_limits<Int>::max());
  engine->makeInvariant<Linear>(*engine, output,
                                std::vector<Int>{aCoef, bCoef, cCoef},
                                std::vector<VarId>{a, b, c});
  engine->close();

  aVal = std::max(aLb, std::min(aUb, aVal));
  bVal = std::max(bLb, std::min(bUb, bVal));
  cVal = std::max(cLb, std::min(cUb, cVal));

  engine->beginMove();
  engine->setValue(a, aVal);
  engine->setValue(b, bVal);
  engine->setValue(c, cVal);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  RC_ASSERT(engine->committedValue(output) ==
            aCoef * aVal + bCoef * bVal + cCoef * cVal);
}

class MockLinear : public Linear {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Linear::registerVars();
  }
  explicit MockLinear(Engine& engine, VarId output, std::vector<VarId> varArray)
      : Linear(engine, output, varArray) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(LinearTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    const size_t numArgs = 10;
    for (size_t value = 1; value <= numArgs; ++value) {
      args.push_back(engine->makeIntVar(static_cast<Int>(value), 1,
                                        static_cast<Int>(numArgs)));
    }
    const VarId modifiedVarId = args.front();
    const VarId output =
        engine->makeIntVar(-10, -100, static_cast<Int>(numArgs * numArgs));
    testNotifications<MockLinear>(
        &engine->makeInvariant<MockLinear>(*engine, output, args),
        {propMode, markingMode, numArgs + 1, modifiedVarId, 5, output});
  }
}

}  // namespace
