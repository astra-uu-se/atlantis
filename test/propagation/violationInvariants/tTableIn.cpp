#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/violationInvariants/tableIn.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;
using ::testing::ContainerEq;

class TableInTest : public InvariantTest {
 public:
  Int numInputVars{3};
  size_t numRows{5};

  Int inputVarLb{0};
  Int inputVarUb{2};

  std::vector<VarViewId> inputVars;
  std::uniform_int_distribution<Int> inputVarDist;

  Int tableLb = inputVarLb-1;
  Int tableUb = inputVarUb+1;

  std::vector<std::vector<Int>> table;
  std::uniform_int_distribution<Int> tableDist;

  VarViewId outputVar{NULL_ID};

  void SetUp() override {
    InvariantTest::SetUp();
    inputVarLb = 0;
    inputVarUb = 2;
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputVars.clear();
  }

  TableIn& generate() {
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);
    inputVars.clear();
    inputVars.reserve(numInputVars);

    _solver->open();
    for (Int i = 0; i < numInputVars; ++i) {
      inputVars.emplace_back(makeIntVar(inputVarLb, inputVarUb, inputVarDist));
    }

    tableDist = std::uniform_int_distribution<Int>(tableLb, tableUb);
    table.resize(numRows, std::vector<Int>(numInputVars));

    for (size_t r = 0; r < numRows; ++r) {
      for (size_t c = 0; c < static_cast<size_t>(numInputVars); ++c) {
        table.at(r).at(c) = tableDist(gen);
      }
    }

    outputVar = _solver->makeIntVar(0, 0, 0);

    TableIn& invariant =
        _solver->makeInvariant<TableIn>(
            *_solver, outputVar,
            std::vector<VarViewId>(inputVars), table);
    _solver->close();
    return invariant;
  }

  Int computeViolation(Timestamp ts) {
    std::vector<Int> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = _solver->value(ts, inputVars.at(i));
    }
    return computeViolation(values);
  }

  Int computeViolation(bool committedValue = false) {
    std::vector<Int> values(inputVars.size());
    for (size_t i = 0; i < inputVars.size(); ++i) {
      values.at(i) = committedValue ? _solver->committedValue(inputVars.at(i))
                                    : _solver->currentValue(inputVars.at(i));
    }
    return computeViolation(values);
  }

  Int computeViolation(const std::vector<Int>& values) {
    std::vector<std::unordered_map<Int, std::vector<size_t>>> valToRows(values.size(), std::unordered_map<Int, std::vector<size_t>>());
    for (size_t c = 0; c < values.size(); ++c) {
      for (size_t r = 0; r < numRows; ++r) {
        if (!valToRows.at(c).contains(table.at(r).at(c))) {
          valToRows.at(c).emplace(table.at(r).at(c), std::vector<size_t>());
        }
        valToRows.at(c).at(table.at(r).at(c)).emplace_back(r);
      }
    }

    std::vector<Int> violations(numRows, values.size());
    for (size_t c = 0; c < values.size(); ++c) {
      if (valToRows.at(c).contains(values.at(c))) {
        for (const auto& row : valToRows.at(c).at(values.at(c))) {
          --violations.at(row);
        }
      }
    }
    return std::ranges::min(violations);
  }

  Int actualViolation(bool committedValue = false) {
      return committedValue ? _solver->committedValue(outputVar)
                                       : _solver->currentValue(outputVar);
  }

  Int actualViolation(Timestamp ts) {
    return _solver->value(ts, outputVar);
  }
};

TEST_F(TableInTest, UpdateBounds) {
  const Int lb = 0;
  const Int ub = 2;

  auto& invariant = generate();
  EXPECT_EQ(_solver->lowerBound(outputVar), 0);

  for (Int aVal = lb; aVal <= ub; ++aVal) {
    _solver->setValue(_solver->currentTimestamp(), inputVars.at(0), aVal);
    for (Int bVal = lb; bVal <= ub; ++bVal) {
      _solver->setValue(_solver->currentTimestamp(), inputVars.at(1), bVal);
      for (Int cVal = lb; cVal <= ub; ++cVal) {
        _solver->setValue(_solver->currentTimestamp(), inputVars.at(2), cVal);
        invariant.updateBounds(false);
        invariant.recompute(_solver->currentTimestamp());
        EXPECT_GE(_solver->currentValue(outputVar), 0);
        EXPECT_LE(_solver->currentValue(outputVar), inputVars.size());
      }
    }
  }
}

TEST_F(TableInTest, Recompute) {
  generateState = GenerateState::LB;

  for (size_t i = 0; i < 5; ++i) {
    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      Int expectedOutput = computeViolation(ts);
      invariant.recompute(ts);
      EXPECT_EQ(expectedOutput, actualViolation(ts));
    }
  }
}

TEST_F(TableInTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  for (size_t b = 0; b < 5; ++b) {
    auto& invariant = generate();

    auto inputVals = makeValVector(inputVars);

    Timestamp ts = _solver->currentTimestamp();

    while (increaseNextVal(inputVars, inputVals) >= 0) {
      ++ts;
      setVarVals(ts, inputVars, inputVals);

      const Int expectedOutput = computeViolation(ts);
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_EQ(expectedOutput, actualViolation(ts));
    }
  }
}

TEST_F(TableInTest, NextInput) {
  numInputVars = 100;
  inputVarLb = -2;
  inputVarUb = 2;

  auto& invariant = generate();

  expectNextInput(inputVars, invariant);
}

TEST_F(TableInTest, NotifyCurrentInputChanged) {
  inputVarLb = -2;
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
      const Int expectedOutput = computeViolation(ts);

      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
    }
  }
}

TEST_F(TableInTest, Commit) {
  numInputVars = 1000;
  inputVarLb = -2;
  inputVarUb = 2;

  auto& invariant = generate();

  std::vector<size_t> indices;
  std::vector<Int> committedValues;

  for (Int i = 0; i < numInputVars; ++i) {
    indices.emplace_back(i);
    committedValues.emplace_back(_solver->committedValue(inputVars.at(i)));
  }

  std::ranges::shuffle(indices.begin(), indices.end(), rng);

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
    Int notifiedOutputValue = _solver->value(ts, outputVar);

    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutputValue, _solver->value(ts, outputVar));

    _solver->commitIf(ts, VarId(inputVars.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputVars.at(i)));
    _solver->commitIf(ts, VarId(outputVar));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutputValue,
                _solver->value(ts + 1, outputVar));
  }
}

RC_GTEST_FIXTURE_PROP(TableInTest, rapidcheck, ()) {
  numInputVars = *rc::gen::inRange(1, 100);
  inputVarLb = -2;
  inputVarUb = 2;


  generate();

  constexpr size_t numCommits = 3;
  constexpr size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    Int expected = computeViolation(true);
    RC_ASSERT(_solver->committedValue(outputVar) == expected);

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
      _solver->query(outputVar);
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      expected = computeViolation();
      RC_ASSERT(_solver->currentValue(outputVar) == expected);
    }
    expected = computeViolation(true);
    RC_ASSERT(_solver->committedValue(outputVar) == expected);
  }
}

class MockTableIn : public TableIn {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    TableIn::registerVars();
  }
  explicit MockTableIn(SolverBase& _solver,
                                     VarViewId violationId,
                                     std::vector<VarViewId>&& inputVars,
                                     const std::vector<std::vector<Int>>& table)
      : TableIn(_solver, violationId,
                              std::move(inputVars), table) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return TableIn::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return TableIn::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          TableIn::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId localId) {
          TableIn::notifyInputChanged(timestamp, localId);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      TableIn::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(TableInTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const Int numinputVars = 10;

    std::vector<VarViewId> inputVars;
    for (Int value = 0; value < numinputVars; ++value) {
      inputVars.push_back(_solver->makeIntVar(0, -100, 100));
    }
    std::vector<std::vector<Int>> table(5, std::vector<Int>(numinputVars));
    for (size_t r = 0; r < table.size(); ++r) {
      for (size_t c = 0; c < table[r].size(); ++c) {
        table[r][c] = -2 + r;
      }
    }
    VarViewId outputVar = _solver->makeIntVar(0, 0, numinputVars);
    const VarViewId modifiedVarId = inputVars.front();
    const VarViewId queryVarId = outputVar;
    testNotifications<MockTableIn>(
        &_solver->makeViolationInvariant<MockTableIn>(
            *_solver, outputVar, std::move(inputVars),
            table),
        {propMode, markingMode, numinputVars + 1, modifiedVarId, 1,
         queryVarId});
  }
}

}  // namespace atlantis::testing
