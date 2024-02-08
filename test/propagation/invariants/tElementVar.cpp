#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../invariantTestHelper.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ElementVarTest : public InvariantTest {
 protected:
  const size_t numValues = 100;
  const Int inputLb = std::numeric_limits<Int>::min();
  const Int inputUb = std::numeric_limits<Int>::max();
  std::vector<Int> offsets{-10, 0, 10};
  std::vector<VarId> inputs;
  std::uniform_int_distribution<Int> inputDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    inputs.resize(numValues, NULL_ID);
    inputDist = std::uniform_int_distribution<Int>(inputLb, inputUb);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputs.clear();
  }

  [[nodiscard]] size_t zeroBasedIndex(const Int indexVal,
                                      const Int offset) const {
    EXPECT_LE(offset, indexVal);
    EXPECT_LT(indexVal - offset, static_cast<Int>(numValues));
    return indexVal - offset;
  }

  VarId getInput(const Int indexVal, const Int offset) {
    return inputs.at(zeroBasedIndex(indexVal, offset));
  }

  Int computeOutput(const Timestamp ts, const VarId index, const Int offset) {
    return computeOutput(ts, solver->value(ts, index), offset);
  }

  Int computeOutput(const Timestamp ts, const Int indexVal, const Int offset) {
    return solver->value(ts, getInput(indexVal, offset));
  }
};

TEST_F(ElementVarTest, UpdateBounds) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = static_cast<Int>(numValues) - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    solver->open();
    const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (auto& input : inputs) {
      input = solver->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = solver->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant = solver->makeInvariant<ElementVar>(
        *solver, outputId, index, std::vector<VarId>(inputs), offset);
    solver->close();

    for (Int minIndex = indexLb; minIndex <= indexUb; ++minIndex) {
      for (Int maxIndex = indexUb; maxIndex >= minIndex; --maxIndex) {
        solver->updateBounds(index, minIndex, maxIndex, false);
        invariant.updateBounds(false);
        Int minVal = std::numeric_limits<Int>::max();
        Int maxVal = std::numeric_limits<Int>::min();
        for (Int indexVal = minIndex; indexVal <= maxIndex; ++indexVal) {
          minVal =
              std::min(minVal, solver->lowerBound(getInput(indexVal, offset)));
          maxVal =
              std::max(maxVal, solver->upperBound(getInput(indexVal, offset)));
        }
        EXPECT_EQ(minVal, solver->lowerBound(outputId));
        EXPECT_EQ(maxVal, solver->upperBound(outputId));
      }
    }
  }
}

TEST_F(ElementVarTest, Recompute) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = static_cast<Int>(numValues) - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    solver->open();
    const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (auto& input : inputs) {
      input = solver->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = solver->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant = solver->makeInvariant<ElementVar>(
        *solver, outputId, index, std::vector<VarId>(inputs), offset);
    solver->close();

    for (Int indexVal = indexLb; indexVal <= indexUb; ++indexVal) {
      solver->setValue(solver->currentTimestamp(), index, indexVal);
      EXPECT_EQ(solver->value(solver->currentTimestamp(), index), indexVal);

      const Int expectedOutput =
          computeOutput(solver->currentTimestamp(), index, offset);
      invariant.recompute(solver->currentTimestamp());
      EXPECT_EQ(solver->value(solver->currentTimestamp(), index), indexVal);

      EXPECT_EQ(expectedOutput,
                solver->value(solver->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ElementVarTest, NotifyInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = static_cast<Int>(numValues) - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    solver->open();
    const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (auto& input : inputs) {
      input = solver->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = solver->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant = solver->makeInvariant<ElementVar>(
        *solver, outputId, index, std::vector<VarId>(inputs), offset);
    solver->close();

    Timestamp ts = solver->currentTimestamp();

    for (Int indexVal = indexLb; indexVal <= indexUb; ++indexVal) {
      ++ts;
      solver->setValue(ts, index, indexVal);
      const Int expectedOutput = computeOutput(ts, index, offset);

      invariant.notifyInputChanged(ts, LocalId(0));
      EXPECT_EQ(expectedOutput, solver->value(ts, outputId));
    }
  }
}

TEST_F(ElementVarTest, NextInput) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = static_cast<Int>(numValues) - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    solver->open();
    const VarId index = solver->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (auto& input : inputs) {
      input = solver->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = solver->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant = solver->makeInvariant<ElementVar>(
        *solver, outputId, index, std::vector<VarId>(inputs), offset);
    solver->close();

    for (Timestamp ts = solver->currentTimestamp() + 1;
         ts < solver->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts), index);
      EXPECT_EQ(invariant.nextInput(ts),
                getInput(solver->value(ts, index), offset));
      EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    }
  }
}

TEST_F(ElementVarTest, NotifyCurrentInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
  Timestamp t0 = solver->currentTimestamp() +
                 (numValues * static_cast<Int>(offsets.size())) + 1;
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = static_cast<Int>(numValues) - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::vector<Int> indexValues(numValues, 0);
    std::iota(indexValues.begin(), indexValues.end(), offset);
    std::shuffle(indexValues.begin(), indexValues.end(), rng);

    solver->open();
    const VarId index =
        solver->makeIntVar(indexValues.back(), indexLb, indexUb);
    for (auto& input : inputs) {
      input = solver->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = solver->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant = solver->makeInvariant<ElementVar>(
        *solver, outputId, index, std::vector<VarId>(inputs), offset);
    solver->close();

    for (size_t i = 0; i < indexValues.size(); ++i) {
      const Int indexVal = indexValues.at(i);
      Timestamp ts = t0 + Timestamp(i);
      EXPECT_EQ(invariant.nextInput(ts), index);
      solver->setValue(ts, index, indexVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, outputId), computeOutput(ts, index, offset));

      const VarId curInput = invariant.nextInput(ts);
      EXPECT_EQ(curInput, getInput(indexVal, offset));

      const Int oldInputVal = solver->value(ts, curInput);
      do {
        solver->setValue(ts, curInput, inputDist(gen));
      } while (solver->value(ts, curInput) == oldInputVal);

      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(solver->value(ts, outputId), computeOutput(ts, index, offset));
    }
  }
}

TEST_F(ElementVarTest, Commit) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = static_cast<Int>(numValues) - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::vector<Int> indexValues(numValues);
    std::iota(indexValues.begin(), indexValues.end(), offset);
    std::shuffle(indexValues.begin(), indexValues.end(), rng);

    solver->open();
    const VarId index =
        solver->makeIntVar(indexValues.back(), indexLb, indexUb);
    for (auto& input : inputs) {
      input = solver->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = solver->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant = solver->makeInvariant<ElementVar>(
        *solver, outputId, index, std::vector<VarId>(inputs), offset);
    solver->close();

    Int committedIndexValue = solver->committedValue(index);

    std::vector<Int> committedInputValues(inputs.size());
    for (size_t i = 0; i < committedInputValues.size(); ++i) {
      committedInputValues.at(i) = solver->committedValue(inputs.at(i));
    }

    for (size_t i = 0; i < indexValues.size(); ++i) {
      const Int indexVal = indexValues.at(i);
      Timestamp ts = solver->currentTimestamp() + Timestamp(i);
      ASSERT_EQ(solver->committedValue(index), committedIndexValue);
      for (size_t j = 0; j < inputs.size(); ++j) {
        ASSERT_EQ(solver->committedValue(inputs.at(j)),
                  committedInputValues.at(j));
      }

      // Change Index
      solver->setValue(ts, index, indexVal);

      // notify index change
      invariant.notifyInputChanged(ts, LocalId(0));

      // incremental value from index
      Int notifiedOutput = solver->value(ts, outputId);
      invariant.recompute(ts);

      ASSERT_EQ(notifiedOutput, solver->value(ts, outputId));

      // Change input
      const VarId curInput = getInput(indexVal, offset);
      const Int oldInputVal = solver->value(ts, curInput);
      do {
        solver->setValue(ts, curInput, inputDist(gen));
      } while (solver->value(ts, curInput) == oldInputVal);

      // notify input change
      invariant.notifyInputChanged(ts, LocalId(indexVal));

      // incremental value from input
      notifiedOutput = solver->value(ts, outputId);
      invariant.recompute(ts);

      ASSERT_EQ(notifiedOutput, solver->value(ts, outputId));

      solver->commitIf(ts, index);
      committedIndexValue = solver->value(ts, index);
      solver->commitIf(ts, curInput);
      committedInputValues.at(zeroBasedIndex(indexVal, offset)) =
          solver->value(ts, curInput);
      solver->commitIf(ts, outputId);

      invariant.commit(ts);
      invariant.recompute(ts + 1);
      ASSERT_EQ(notifiedOutput, solver->value(ts + 1, outputId));
    }
  }
}

class MockElementVar : public ElementVar {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    ElementVar::registerVars();
  }
  explicit MockElementVar(SolverBase& solver, VarId output, VarId index,
                          std::vector<VarId>&& varArray, Int offset)
      : ElementVar(solver, output, index, std::move(varArray), offset) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return ElementVar::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return ElementVar::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          ElementVar::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          ElementVar::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      ElementVar::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ElementVarTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!solver->isOpen()) {
      solver->open();
    }
    std::vector<VarId> args;
    const size_t numArgs = 10;
    args.reserve(numArgs);
    for (size_t value = 0; value < numArgs; ++value) {
      args.push_back(solver->makeIntVar(static_cast<Int>(value), -100, 100));
    }
    VarId idx = solver->makeIntVar(0, 0, static_cast<Int>(numArgs) - 1);
    VarId output = solver->makeIntVar(-10, -100, 100);
    testNotifications<MockElementVar>(
        &solver->makeInvariant<MockElementVar>(*solver, output, idx,
                                               std::move(args), 1),
        {propMode, markingMode, 3, idx, 5, output});
  }
}

}  // namespace atlantis::testing
