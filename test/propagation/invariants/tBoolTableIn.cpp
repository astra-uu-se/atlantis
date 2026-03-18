#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/boolTableIn.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;
using ::testing::ContainerEq;

class BoolTableTestIn : public InvariantTest {
 public:
  Int numInputVars{3};
  size_t numRows{5};

  Int inputVarLb{0};
  Int inputVarUb{2};

  std::vector<VarViewId> inputVars;
  std::uniform_int_distribution<Int> inputVarDist;

  std::vector<std::vector<bool>> table;
  std::uniform_int_distribution<unsigned char> tableDist;

  std::vector<VarViewId> outputVars;

  void SetUp() override {
    InvariantTest::SetUp();
    inputVarLb = 0;
    inputVarUb = 2;
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputVars.clear();
    outputVars.clear();
  }

  BoolTableIn& generate() {
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);
    inputVars.clear();
    inputVars.reserve(numInputVars);

    _solver->open();
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(makeIntVar(inputVarLb, inputVarUb, inputVarDist));
    }

    tableDist = std::uniform_int_distribution<unsigned char>(0, 1);
    table.resize(numRows, std::vector<bool>(numInputVars));

    for (size_t r = 0; r < numRows; ++r) {
      for (size_t c = 0; c < static_cast<size_t>(numInputVars); ++c) {
        table.at(r).at(c) = tableDist(gen) == 0;
      }
    }

    outputVars.clear();
    outputVars.reserve(numRows);
    for (size_t i = 0; i < numRows; ++i) {
      outputVars.emplace_back(_solver->makeIntVar(0, 0, 0));
    }

    BoolTableIn& invariant =
        _solver->makeInvariant<BoolTableIn>(
            *_solver, std::vector<VarViewId>(outputVars),
            std::vector<VarViewId>(inputVars), table);
    _solver->close();
    return invariant;
  }

  std::vector<Int> computeViolations(const Timestamp ts) {
    std::vector<bool> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = _solver->value(ts, inputVars.at(i)) == 0;
    }
    return computeViolations(values);
  }

  std::vector<Int> computeViolations(const bool committedValue = false) {
    std::vector<bool> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = (committedValue ? _solver->committedValue(inputVars.at(i))
                                    : _solver->currentValue(inputVars.at(i))) == 0;
    }
    return computeViolations(values);
  }

  std::vector<Int> computeViolations(const std::vector<bool>& values) {
    std::vector<Int> violations(numRows, 0);
    for (size_t r = 0; r < numRows; ++r) {
      for (size_t c = 0; c < values.size(); ++c) {
        violations.at(r) += (table.at(r).at(c) != values.at(c)) ? 1 : 0;
      }
    }
    return violations;
  }

  std::vector<Int> actualViolations(const bool committedValue = false) {
    std::vector<Int> vals;
    vals.reserve(outputVars.size());
    for (const auto& varId : outputVars) {
      vals.emplace_back(committedValue ? _solver->committedValue(varId)
                                       : _solver->currentValue(varId));
    }
    return vals;
  }

  std::vector<Int> actualViolations(Timestamp ts) {
    std::vector<Int> vals;
    vals.reserve(outputVars.size());
    for (const auto& varId : outputVars) {
      vals.emplace_back(_solver->value(ts, varId));
    }
    return vals;
  }
};

TEST_F(BoolTableTestIn, UpdateBounds) {
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

TEST_F(BoolTableTestIn, Recompute) {
  generateState = GenerateState::LB;

  for (size_t i = 0; i < 5; ++i) {
    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const std::vector<Int> expectedOutputs = computeViolations(ts);
      invariant.recompute(ts);
      EXPECT_THAT(expectedOutputs, ContainerEq(actualViolations(ts)));
    }
  }
}

TEST_F(BoolTableTestIn, NotifyInputChanged) {
  generateState = GenerateState::LB;

  for (size_t b = 0; b < 5; ++b) {
    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const std::vector<Int> expectedOutputs = computeViolations(ts);
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_THAT(expectedOutputs, ContainerEq(actualViolations(ts)));
    }
  }
}

TEST_F(BoolTableTestIn, NextInput) {
  numInputVars = 100;
  inputVarLb = 0;
  inputVarUb = 2;

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(BoolTableTestIn, NotifyCurrentInputChanged) {
  inputVarLb = 0;
  inputVarUb = 2;

  auto& invariant = generate();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(ts, varId, inputVarDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      const auto expectedCounts = computeViolations(ts);

      invariant.notifyCurrentInputChanged(ts);
      for (size_t i = 0; i < outputVars.size(); ++i) {
        EXPECT_EQ(expectedCounts.at(i), _solver->value(ts, outputVars.at(i)));
      }
    }
  }
}

TEST_F(BoolTableTestIn, Commit) {
  numInputVars = 1000;
  inputVarLb = 0;
  inputVarUb = 2;

  auto& invariant = generate();

  std::vector<size_t> indices;
  std::vector<Int> committedValues;

  for (Int i = 0; i < numInputVars; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(_solver->committedValue(inputVars.at(i)));
  }

  std::ranges::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> notifiedOutputValues(numRows, -1);

  for (const size_t i : indices) {
    const Timestamp ts = _solver->currentTimestamp() + Timestamp(i);
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

RC_GTEST_FIXTURE_PROP(BoolTableTestIn, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);
  inputVarLb = 0;
  inputVarUb = 2;


  generate();

  constexpr size_t numCommits = 3;
  constexpr size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    std::vector<Int> expected = computeViolations(true);
    RC_ASSERT(expected.size() == outputVars.size());
    for (size_t i = 0; i < numRows; ++i) {
      RC_ASSERT(_solver->committedValue(outputVars.at(i)) == expected.at(i));
    }

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      for (const auto& var : inputVars) {
        if (randBool()) {
          _solver->setValue(var, inputVarDist(gen));
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
      expected = computeViolations();
      RC_ASSERT(expected.size() == outputVars.size());
      for (size_t i = 0; i < numRows; ++i) {
        RC_ASSERT(_solver->currentValue(outputVars.at(i)) == expected.at(i));
      }
    }
    expected = computeViolations(true);
    RC_ASSERT(expected.size() == outputVars.size());
    for (size_t i = 0; i < numRows; ++i) {
      RC_ASSERT(_solver->committedValue(outputVars.at(i)) == expected.at(i));
    }
  }
}

class MockBoolTableIn : public BoolTableIn {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    BoolTableIn::registerVars();
  }
  explicit MockBoolTableIn(SolverBase& _solver,
                                     std::vector<VarViewId>&& rowViolations,
                                     std::vector<VarViewId>&& inputVars,
                                     const std::vector<std::vector<bool>>& table)
      : BoolTableIn(_solver, std::move(rowViolations),
                              std::move(inputVars), table) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return BoolTableIn::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return BoolTableIn::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          BoolTableIn::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId localId) {
          BoolTableIn::notifyInputChanged(timestamp, localId);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      BoolTableIn::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(BoolTableTestIn, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const Int numinputVars = 10;

    std::vector<VarViewId> inputVars;
    for (Int value = 0; value < numinputVars; ++value) {
      inputVars.push_back(_solver->makeIntVar(0, 0, 2));
    }
    std::vector<std::vector<bool>> table(5, std::vector<bool>(numinputVars));
    for (size_t r = 0; r < table.size(); ++r) {
      for (size_t c = 0; c < table[r].size(); ++c) {
        table[r][c] = (r + c) % 2 == 0;
      }
    }
    std::vector<VarViewId> outputVars;
    for (size_t i = 0; i < table.size(); ++i) {
      outputVars.push_back(_solver->makeIntVar(0, 0, numinputVars));
    }
    const VarViewId modifiedVarId = inputVars.front();
    const VarViewId queryVarId = outputVars.front();
    testNotifications<MockBoolTableIn>(
        &_solver->makeInvariant<MockBoolTableIn>(
            *_solver, std::move(outputVars), std::move(inputVars),
            table),
        {propMode, markingMode, numinputVars + 1, modifiedVarId, 1,
         queryVarId});
  }
}

}  // namespace atlantis::testing
