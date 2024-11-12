#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ElementVarTest : public InvariantTest {
 protected:
  Int numDynamicVars{100};
  Int offset{1};

  std::vector<Int> offsets{-10, 0, 10};
  std::uniform_int_distribution<Int> dynamicInputDist;
  std::uniform_int_distribution<Int> indexDist;

  std::vector<VarViewId> dynamicInputs;
  VarViewId indexVar{NULL_ID};
  VarViewId outputVar{NULL_ID};

  Int indexLb() const { return offset; }
  Int indexUb() const { return offset + numDynamicVars - 1; }

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    dynamicInputDist = std::uniform_int_distribution<Int>(
        std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  }

  void TearDown() override {
    InvariantTest::TearDown();
    dynamicInputs.clear();
  }

  ElementVar& generate() {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    dynamicInputs.resize(numDynamicVars, NULL_ID);
    for (Int i = 0; i < numDynamicVars; ++i) {
      dynamicInputs.at(i) =
          makeIntVar(std::numeric_limits<Int>::min(),
                     std::numeric_limits<Int>::max(), dynamicInputDist);
    }

    indexDist = std::uniform_int_distribution<Int>(indexLb(), indexUb());

    indexVar = makeIntVar(indexLb(), indexUb(), indexDist);
    outputVar = _solver->makeIntVar(0, 0, 0);

    ElementVar& invariant = _solver->makeInvariant<ElementVar>(
        *_solver, outputVar, indexVar, std::vector<VarViewId>(dynamicInputs),
        offset);
    _solver->close();

    return invariant;
  }

  [[nodiscard]] size_t zeroBasedIndex(Int indexVal) const {
    EXPECT_LE(offset, indexVal);
    EXPECT_LT(indexVal - offset, static_cast<Int>(numDynamicVars));
    return indexVal - offset;
  }

  VarViewId getInput(Int indexVal) {
    return dynamicInputs.at(zeroBasedIndex(indexVal));
  }

  Int computeOutput(Timestamp ts) {
    return computeOutput(ts, _solver->value(ts, indexVar));
  }

  Int computeOutput(Timestamp ts, Int indexVal) {
    return _solver->value(ts, getInput(indexVal));
  }

  Int computeOutput(bool committedValue = false) {
    return committedValue ? _solver->committedValue(
                                getInput(_solver->committedValue(indexVar)))
                          : _solver->currentValue(
                                getInput(_solver->currentValue(indexVar)));
  }
};

TEST_F(ElementVarTest, UpdateBounds) {
  EXPECT_LE(std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  for (const Int o : offsets) {
    offset = o;

    auto& invariant = generate();

    for (Int minIndex = indexLb(); minIndex <= indexUb(); ++minIndex) {
      for (Int maxIndex = indexUb(); maxIndex >= minIndex; --maxIndex) {
        _solver->updateBounds(VarId(indexVar), minIndex, maxIndex, false);
        invariant.updateBounds(false);
        Int minVal = std::numeric_limits<Int>::max();
        Int maxVal = std::numeric_limits<Int>::min();
        for (Int indexVal = minIndex; indexVal <= maxIndex; ++indexVal) {
          minVal = std::min(minVal, _solver->lowerBound(getInput(indexVal)));
          maxVal = std::max(maxVal, _solver->upperBound(getInput(indexVal)));
        }
        EXPECT_EQ(minVal, _solver->lowerBound(outputVar));
        EXPECT_EQ(maxVal, _solver->upperBound(outputVar));
      }
    }
  }
}

TEST_F(ElementVarTest, Recompute) {
  generateState = GenerateState::LB;

  EXPECT_LE(std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  for (const Int o : offsets) {
    offset = o;

    auto& invariant = generate();

    Timestamp ts = _solver->currentTimestamp();

    for (Int indexVal = indexLb(); indexVal <= indexUb(); ++indexVal) {
      ++ts;
      _solver->setValue(_solver->currentTimestamp(), indexVar, indexVal);
      EXPECT_EQ(_solver->currentValue(indexVar), indexVal);

      const Int expectedOutput = computeOutput(ts);
      invariant.recompute(ts);
      EXPECT_EQ(_solver->currentValue(indexVar), indexVal);

      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(ElementVarTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  for (const Int o : offsets) {
    offset = o;

    auto& invariant = generate();

    Timestamp ts = _solver->currentTimestamp();

    for (Int indexVal = indexLb(); indexVal <= indexUb(); ++indexVal) {
      ++ts;
      _solver->setValue(ts, indexVar, indexVal);

      const Int expectedOutput = computeOutput(ts);
      invariant.notifyInputChanged(ts, LocalId(numDynamicVars));
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(ElementVarTest, NextInput) {
  EXPECT_LE(std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  for (const Int o : offsets) {
    offset = o;

    auto& invariant = generate();

    for (Timestamp ts = _solver->currentTimestamp() + 1;
         ts < _solver->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts), indexVar);
      EXPECT_EQ(invariant.nextInput(ts),
                getInput(_solver->value(ts, indexVar)));
      EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    }
  }
}

TEST_F(ElementVarTest, NotifyCurrentInputChanged) {
  EXPECT_LE(std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  Timestamp t0 = _solver->currentTimestamp() +
                 (numDynamicVars * static_cast<Int>(offsets.size())) + 1;
  for (const Int o : offsets) {
    offset = o;

    auto& invariant = generate();

    std::vector<Int> indexValues(numDynamicVars);
    std::iota(indexValues.begin(), indexValues.end(), indexLb());
    std::shuffle(indexValues.begin(), indexValues.end(), rng);

    for (size_t i = 0; i < indexValues.size(); ++i) {
      const Int indexVal = indexValues.at(i);
      Timestamp ts = t0 + Timestamp(i);
      EXPECT_EQ(invariant.nextInput(ts), indexVar);
      _solver->setValue(ts, indexVar, indexVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));

      const VarViewId curInput = invariant.nextInput(ts);
      EXPECT_EQ(curInput, getInput(indexVal));

      const Int oldInputVal = _solver->value(ts, curInput);
      do {
        _solver->setValue(ts, curInput, dynamicInputDist(gen));
      } while (_solver->value(ts, curInput) == oldInputVal);

      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(ElementVarTest, Commit) {
  EXPECT_LE(std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max());
  for (const Int o : offsets) {
    offset = o;

    auto& invariant = generate();

    Int committedIndexValue = _solver->committedValue(indexVar);

    std::vector<Int> committedInputValues(dynamicInputs.size());
    for (size_t i = 0; i < committedInputValues.size(); ++i) {
      committedInputValues.at(i) = _solver->committedValue(dynamicInputs.at(i));
    }

    std::vector<Int> indexValues(numDynamicVars);
    std::iota(indexValues.begin(), indexValues.end(), indexLb());
    std::shuffle(indexValues.begin(), indexValues.end(), rng);

    for (size_t i = 0; i < indexValues.size(); ++i) {
      const Int indexVal = indexValues.at(i);
      Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
      ASSERT_EQ(_solver->committedValue(indexVar), committedIndexValue);
      for (size_t j = 0; j < dynamicInputs.size(); ++j) {
        ASSERT_EQ(_solver->committedValue(dynamicInputs.at(j)),
                  committedInputValues.at(j));
      }

      // Change Index
      _solver->setValue(ts, indexVar, indexVal);

      // notify indexVar change
      invariant.notifyInputChanged(ts, LocalId(0));

      // incremental value from indexVar
      Int notifiedOutput = _solver->value(ts, outputVar);
      invariant.recompute(ts);

      ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

      // Change input
      const VarViewId curInput = getInput(indexVal);
      const Int oldInputVal = _solver->value(ts, curInput);
      do {
        _solver->setValue(ts, curInput, dynamicInputDist(gen));
      } while (_solver->value(ts, curInput) == oldInputVal);

      // notify input change
      invariant.notifyInputChanged(ts, LocalId(indexVal));

      // incremental value from input
      notifiedOutput = _solver->value(ts, outputVar);
      invariant.recompute(ts);

      ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

      _solver->commitIf(ts, VarId(indexVar));
      committedIndexValue = _solver->value(ts, indexVar);
      _solver->commitIf(ts, VarId(curInput));
      committedInputValues.at(zeroBasedIndex(indexVal)) =
          _solver->value(ts, curInput);
      _solver->commitIf(ts, VarId(outputVar));

      invariant.commit(ts);
      invariant.recompute(ts + 1);
      ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
    }
  }
}

RC_GTEST_FIXTURE_PROP(ElementVarTest, rapidcheck, ()) {
  numDynamicVars = *rc::gen::inRange<Int>(1, 1000);

  offset = *rc::gen::inRange(std::numeric_limits<Int>::min() + 200,
                             std::numeric_limits<Int>::max() - 200);

  std::vector<VarViewId> inputVars;
  inputVars.reserve(numDynamicVars + 1);

  generate();

  std::vector<std::uniform_int_distribution<Int>> dists;
  dists.reserve(numDynamicVars + 1);

  for (const VarViewId& dynId : dynamicInputs) {
    inputVars.emplace_back(dynId);
    dists.emplace_back(dynamicInputDist);
  }

  inputVars.emplace_back(indexVar);
  dists.emplace_back(indexDist);

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (size_t i = 0; i < inputVars.size(); ++i) {
        if (randBool()) {
          _solver->setValue(inputVars.at(i), dists.at(i)(gen));
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

class MockElementVar : public ElementVar {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    ElementVar::registerVars();
  }
  explicit MockElementVar(SolverBase& solver, VarViewId output,
                          VarViewId indexVar, std::vector<VarViewId>&& varArray,
                          Int offset)
      : ElementVar(solver, output, indexVar, std::move(varArray), offset) {
    EXPECT_TRUE(output.isVar());

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
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ElementVarTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<VarViewId> args;
    const size_t numArgs = 10;
    args.reserve(numArgs);
    for (size_t value = 0; value < numArgs; ++value) {
      args.push_back(_solver->makeIntVar(static_cast<Int>(value), -100, 100));
    }
    VarViewId idx = _solver->makeIntVar(0, 0, static_cast<Int>(numArgs) - 1);
    VarViewId output = _solver->makeIntVar(-10, -100, 100);
    testNotifications<MockElementVar>(
        &_solver->makeInvariant<MockElementVar>(*_solver, output, idx,
                                                std::move(args), 1),
        {propMode, markingMode, 3, idx, 5, output});
  }
}

}  // namespace atlantis::testing
