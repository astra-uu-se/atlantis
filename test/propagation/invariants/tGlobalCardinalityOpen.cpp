#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;
using ::testing::ContainerEq;

class GlobalCardinalityOpenTest : public InvariantTest {
 public:
  Int numInputVars{3};

  Int inputVarLb{-2};
  Int inputVarUb{2};

  std::vector<VarViewId> inputVars;
  std::uniform_int_distribution<Int> inputVarDist;

  std::vector<Int> cover;
  std::vector<VarViewId> outputVars;

  void SetUp() override {
    InvariantTest::SetUp();
    inputVarLb = 0;
    inputVarUb = 2;
    cover = std::vector<Int>{0, 2};
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputVars.clear();
    outputVars.clear();
    cover.clear();
  }

  GlobalCardinalityOpen& generate() {
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);
    inputVars.clear();
    inputVars.reserve(numInputVars);

    _solver->open();
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(makeIntVar(inputVarLb, inputVarUb, inputVarDist));
    }

    if (cover.empty()) {
      cover.reserve(2);
      cover.emplace_back(inputVarLb);
      if (inputVarLb != inputVarUb) {
        cover.emplace_back(inputVarUb);
      }
    }

    outputVars.clear();
    outputVars.reserve(cover.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      outputVars.emplace_back(_solver->makeIntVar(0, 0, 0));
    }

    GlobalCardinalityOpen& invariant =
        _solver->makeInvariant<GlobalCardinalityOpen>(
            *_solver, std::vector<VarViewId>(outputVars),
            std::vector<VarViewId>(inputVars), std::vector<Int>(cover));
    _solver->close();
    return invariant;
  }

  std::vector<Int> computeOutputs(Timestamp ts) {
    std::vector<Int> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = _solver->value(ts, inputVars.at(i));
    }
    return computeOutputs(values);
  }

  std::vector<Int> computeOutputs(bool committedValue = false) {
    std::vector<Int> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = committedValue ? _solver->committedValue(inputVars.at(i))
                                    : _solver->currentValue(inputVars.at(i));
    }
    return computeOutputs(values);
  }

  std::vector<Int> computeOutputs(const std::vector<Int>& values) {
    std::unordered_map<Int, size_t> coverToIndex;
    for (size_t i = 0; i < cover.size(); ++i) {
      coverToIndex.emplace(cover.at(i), i);
    }

    std::vector<Int> counts(cover.size(), 0);
    for (const Int val : values) {
      if (coverToIndex.contains(val)) {
        counts.at(coverToIndex.at(val)) += 1;
      }
    }
    return counts;
  }

  std::vector<Int> actualOutputs(bool committedValue = false) {
    std::vector<Int> vals;
    vals.reserve(outputVars.size());
    for (const auto& varId : outputVars) {
      vals.emplace_back(committedValue ? _solver->committedValue(varId)
                                       : _solver->currentValue(varId));
    }
    return vals;
  }

  std::vector<Int> actualOutputs(Timestamp ts) {
    std::vector<Int> vals;
    vals.reserve(outputVars.size());
    for (const auto& varId : outputVars) {
      vals.emplace_back(_solver->value(ts, varId));
    }
    return vals;
  }
};

TEST_F(GlobalCardinalityOpenTest, UpdateBounds) {
  const Int lb = 0;
  const Int ub = 2;

  auto& invariant = generate();
  for (const VarViewId& output : outputVars) {
    EXPECT_EQ(_solver->lowerBound(output), 0);
  }

  for (Int aVal = lb; aVal <= ub; ++aVal) {
    _solver->setValue(_solver->currentTimestamp(), inputVars.at(0), aVal);
    for (Int bVal = lb; bVal <= ub; ++bVal) {
      _solver->setValue(_solver->currentTimestamp(), inputVars.at(1), bVal);
      for (Int cVal = lb; cVal <= ub; ++cVal) {
        _solver->setValue(_solver->currentTimestamp(), inputVars.at(2), cVal);
        invariant.updateBounds(false);
        invariant.recompute(_solver->currentTimestamp());
        for (const VarViewId& output : outputVars) {
          EXPECT_GE(_solver->currentValue(output), 0);
          EXPECT_LE(_solver->currentValue(output), inputVars.size());
        }
      }
    }
  }
}

TEST_F(GlobalCardinalityOpenTest, Recompute) {
  generateState = GenerateState::LB;

  std::vector<std::pair<Int, Int>> boundVec{
      {-10004, -10000}, {-2, 2}, {10000, 10002}};

  std::vector<std::vector<Int>> coverVec{{std::vector<Int>{-10003, -10002}},
                                         {std::vector<Int>{-2, 2}},
                                         {std::vector<Int>{10000, 10002}}};

  for (size_t i = 0; i < boundVec.size(); ++i) {
    inputVarLb = boundVec[i].first;
    inputVarUb = boundVec[i].second;
    cover = coverVec[i];

    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const std::vector<Int> expectedOutputs = computeOutputs(ts);
      invariant.recompute(ts);
      EXPECT_THAT(expectedOutputs, ContainerEq(actualOutputs(ts)));
    }
  }
}

TEST_F(GlobalCardinalityOpenTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  std::vector<std::pair<Int, Int>> boundVec{
      {-10002, -10000}, {-1, 1}, {10000, 10002}};

  std::vector<std::vector<Int>> coverVec{{std::vector<Int>{-10003, -10002}},
                                         {std::vector<Int>{-2, 2}},
                                         {std::vector<Int>{10000, 10002}}};

  for (size_t b = 0; b < boundVec.size(); ++b) {
    inputVarLb = boundVec[b].first;
    inputVarUb = boundVec[b].second;
    cover = coverVec[b];

    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const std::vector<Int> expectedOutputs = computeOutputs(ts);
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_THAT(expectedOutputs, ContainerEq(actualOutputs(ts)));
    }
  }
}

TEST_F(GlobalCardinalityOpenTest, NextInput) {
  numInputVars = 100;
  inputVarLb = 0;
  inputVarUb = numInputVars - 1;
  const Int coverSize = 10;

  cover.resize(coverSize);
  std::iota(cover.begin(), cover.end(), inputVarLb);

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(GlobalCardinalityOpenTest, NotifyCurrentInputChanged) {
  const Int numinputVars = 100;
  inputVarLb = 0;
  inputVarUb = numinputVars - 1;
  const Int coverSize = 10;

  cover.resize(coverSize);
  std::iota(cover.begin(), cover.end(), inputVarLb);

  auto& invariant = generate();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, inputVarDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      const auto expectedCounts = computeOutputs(ts);

      invariant.notifyCurrentInputChanged(ts);
      for (size_t i = 0; i < outputVars.size(); ++i) {
        EXPECT_EQ(expectedCounts.at(i), _solver->value(ts, outputVars.at(i)));
      }
    }
  }
}

TEST_F(GlobalCardinalityOpenTest, Commit) {
  numInputVars = 1000;
  inputVarLb = 0;
  inputVarUb = numInputVars - 1;
  const Int coverSize = 10;

  cover.resize(coverSize);
  std::iota(cover.begin(), cover.end(), inputVarLb);

  auto& invariant = generate();

  std::vector<size_t> indices;
  std::vector<Int> committedValues;

  for (Int i = 0; i < numInputVars; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(_solver->committedValue(inputVars.at(i)));
  }

  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> notifiedOutputValues(coverSize, -1);

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
    for (size_t j = 0; j < outputVars.size(); ++j) {
      notifiedOutputValues.at(j) = _solver->value(ts, outputVars.at(j));
    }
    invariant.recompute(ts);

    for (size_t j = 0; j < outputVars.size(); ++j) {
      ASSERT_EQ(notifiedOutputValues.at(j),
                _solver->value(ts, outputVars.at(j)));
    }

    _solver->commitIf(ts, VarId(inputVars.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputVars.at(i)));
    for (const VarViewId& o : outputVars) {
      _solver->commitIf(ts, VarId(o));
    }

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    for (size_t j = 0; j < outputVars.size(); ++j) {
      ASSERT_EQ(notifiedOutputValues.at(j),
                _solver->value(ts + 1, outputVars.at(j)));
    }
  }
}

RC_GTEST_FIXTURE_PROP(GlobalCardinalityOpenTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);
  inputVarLb =
      *rc::gen::inRange(std::numeric_limits<Int>::min(),
                        std::numeric_limits<Int>::max() - numInputVars);
  inputVarUb = *rc::gen::inRange(inputVarLb, std::numeric_limits<Int>::max());

  const size_t coverSize = *rc::gen::inRange(1, 100);

  cover = *rc::gen::unique<std::vector<Int>>(
      coverSize, rc::gen::inRange(inputVarLb, inputVarLb + 1001));

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    std::vector<Int> expected = computeOutputs(true);
    RC_ASSERT(expected.size() == outputVars.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      RC_ASSERT(_solver->committedValue(outputVars.at(i)) == expected.at(i));
    }

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
      for (const auto& outputVar : outputVars) {
        _solver->query(outputVar);
      }
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      expected = computeOutputs();
      RC_ASSERT(expected.size() == outputVars.size());
      for (size_t i = 0; i < cover.size(); ++i) {
        RC_ASSERT(_solver->currentValue(outputVars.at(i)) == expected.at(i));
      }
    }
    expected = computeOutputs(true);
    RC_ASSERT(expected.size() == outputVars.size());
    for (size_t i = 0; i < cover.size(); ++i) {
      RC_ASSERT(_solver->committedValue(outputVars.at(i)) == expected.at(i));
    }
  }
}

class MockGlobalCardinalityOpen : public GlobalCardinalityOpen {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    GlobalCardinalityOpen::registerVars();
  }
  explicit MockGlobalCardinalityOpen(SolverBase& _solver,
                                     std::vector<VarViewId>&& outputVars,
                                     std::vector<VarViewId>&& inputVars,
                                     std::vector<Int>&& cover)
      : GlobalCardinalityOpen(_solver, std::move(outputVars),
                              std::move(inputVars), std::move(cover)) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return GlobalCardinalityOpen::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return GlobalCardinalityOpen::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          GlobalCardinalityOpen::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId localId) {
          GlobalCardinalityOpen::notifyInputChanged(timestamp, localId);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      GlobalCardinalityOpen::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(GlobalCardinalityOpenTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const Int numinputVars = 10;

    std::vector<VarViewId> inputVars;
    for (Int value = 0; value < numinputVars; ++value) {
      inputVars.push_back(_solver->makeIntVar(0, -100, 100));
    }
    std::vector<Int> cover{1, 2, 3};
    std::vector<VarViewId> outputVars;
    for (size_t i = 0; i < cover.size(); ++i) {
      outputVars.push_back(_solver->makeIntVar(0, 0, numinputVars));
    }
    const VarViewId modifiedVarId = inputVars.front();
    const VarViewId queryVarId = outputVars.front();
    testNotifications<MockGlobalCardinalityOpen>(
        &_solver->makeInvariant<MockGlobalCardinalityOpen>(
            *_solver, std::move(outputVars), std::move(inputVars),
            std::move(cover)),
        {propMode, markingMode, numinputVars + 1, modifiedVarId, 1,
         queryVarId});
  }
}

}  // namespace atlantis::testing
