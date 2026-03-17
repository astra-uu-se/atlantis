#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/table.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;
using ::testing::ContainerEq;

class TableTest : public InvariantTest {
 public:
  size_t numRows{5};
  size_t numOutputVars{3};
  size_t inputColIndex{0};

  Int varLb{-2};
  Int varUb{2};

  VarViewId inputVar{NULL_ID};

  std::uniform_int_distribution<Int> varDist;

  std::vector<std::vector<Int>> table;

  std::vector<VarViewId> outputVars;

  void TearDown() override {
    InvariantTest::TearDown();
    outputVars.clear();
  }

  Table& generate() {
    varDist = std::uniform_int_distribution<Int>(varLb, varUb);

    _solver->open();
    inputVar = makeIntVar(varLb, varUb, varDist);

    EXPECT_GE(varUb - varLb + 1, numRows);
    std::vector<Int> inputColVals(varUb - varLb + 1);
    std::iota(inputColVals.begin(), inputColVals.end(), varLb);
    std::ranges::shuffle(inputColVals.begin(), inputColVals.end(), rng);
    inputColVals.resize(numRows);

    table.clear();
    table.resize(numRows, std::vector<Int>(numOutputVars + 1));
    for (size_t r = 0; r < numRows; ++r) {
      for (size_t c = 0; c <= numOutputVars; ++c) {
        table.at(r).at(c) = c == inputColIndex ? inputColVals.at(r) : varDist(gen);
      }
    }

    outputVars.clear();
    outputVars.reserve(numOutputVars);
    for (size_t i = 0; i < numOutputVars; ++i) {
      outputVars.emplace_back(_solver->makeIntVar(0, 0, 0));
    }

    Table& invariant =
        _solver->makeInvariant<Table>(
            *_solver, std::vector<VarViewId>(outputVars),
            inputVar, std::vector<std::vector<Int>>{table});
    _solver->close();
    return invariant;
  }

  std::vector<Int> expectedRow(const Timestamp ts) {
    return expectedRow(_solver->value(ts, inputVar));
  }

  std::vector<Int> expectedRow(bool committedValue = false) {
    return expectedRow(committedValue ? _solver->committedValue(inputVar)
                                    : _solver->currentValue(inputVar));
  }

  std::vector<Int> expectedRow(const Int val) {
    size_t row = 0;
    for (size_t r = 0; r < numRows; ++r) {
      if (table.at(r).at(inputColIndex) == val) {
        row = r;
        break;
      }
    }
    return {table.at(row)};
  }

  std::vector<Int> actualRow(const Timestamp ts) {
    std::vector<Int> vals(numOutputVars + 1);
    for (size_t c = 0; c < numOutputVars + 1; ++c) {
      const VarViewId vId = c == inputColIndex ? inputVar : outputVars.at(c - (c < inputColIndex ? 0 : 1));
      vals.at(c) = _solver->value(ts, vId);
    }
    return vals;
  }

  std::vector<Int> actualRow(bool committedValue = false) {
    std::vector<Int> vals;
    vals.reserve(numOutputVars + 1);
    for (size_t c = 0; c < numOutputVars + 1; ++c) {
      const VarViewId vId = c == inputColIndex ? inputVar : outputVars.at(c - (c < inputColIndex ? 0 : 1));
      vals.at(c) = vals.emplace_back(committedValue ? _solver->committedValue(vId)
                                    : _solver->currentValue(vId));
    }
    return vals;
  }
};

TEST_F(TableTest, UpdateBounds) {
  auto& invariant = generate();
  for (const VarViewId& output : outputVars) {
    EXPECT_GE(_solver->currentValue(output), varLb);
    EXPECT_LE(_solver->currentValue(output), varUb);
  }

  for (Int val = varLb; val <= varUb; ++val) {
    _solver->setValue(_solver->currentTimestamp(), inputVar, val);
    invariant.updateBounds(false);
    invariant.recompute(_solver->currentTimestamp());
    for (const VarViewId& output : outputVars) {
      EXPECT_GE(_solver->currentValue(output), varLb);
      EXPECT_LE(_solver->currentValue(output), varUb);
    }
  }
}

TEST_F(TableTest, Recompute) {
  generateState = GenerateState::LB;

  for (size_t i = 0; i < 5; ++i) {
    auto& invariant = generate();

    Timestamp ts = _solver->currentTimestamp();

    for (Int val = varLb; val <= varUb; ++val) {
      ++ts;
      _solver->setValue(ts, inputVar, val);

      const std::vector<Int> expectedOutputs = expectedRow(val);
      invariant.recompute(ts);
      const std::vector<Int> actualOutputs = actualRow(ts);
      EXPECT_THAT(expectedOutputs, ContainerEq(actualOutputs));
    }
  }
}

TEST_F(TableTest, NotifyInputChanged) {
  for (size_t b = 0; b < 5; ++b) {
    auto& invariant = generate();

    Timestamp ts = _solver->currentTimestamp();

    for (Int val = varLb; val <= varUb; ++val) {
      ++ts;
      _solver->setValue(ts, inputVar, val);

      const std::vector<Int> expectedOutputs = expectedRow(val);
      std::vector<VarViewId> inputVars{inputVar};
      notifyInputsChanged(ts, invariant, inputVars);
      EXPECT_THAT(expectedOutputs, ContainerEq(actualRow(ts)));
    }
  }
}

TEST_F(TableTest, NextInput) {
  numOutputVars = 100;
  varLb = -2;
  varUb = 2;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{inputVar};
  expectNextInput(inputVars, invariant);
}

TEST_F(TableTest, NotifyCurrentInputChanged) {
  varLb = -2;
  varUb = 2;

  auto& invariant = generate();

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    EXPECT_EQ(invariant.nextInput(ts), inputVar);
    const Int oldVal = _solver->value(ts, inputVar);
    do {
      _solver->setValue(ts, inputVar, varDist(gen));
    } while (_solver->value(ts, inputVar) == oldVal);
    const auto expected = expectedRow(ts);
    invariant.notifyCurrentInputChanged(ts);
    const auto actual = actualRow(ts);
    EXPECT_THAT(expected, ContainerEq(actual));

  }
}

RC_GTEST_FIXTURE_PROP(TableTest, rapidcheck, ()) {
  numOutputVars = *rc::gen::inRange(1, 100);
  varLb = -2;
  varUb = 2;

  generate();

  constexpr size_t numCommits = 3;
  constexpr size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    std::vector<Int> expected = expectedRow(true);
    std::vector<Int> actual = actualRow(true);
    RC_ASSERT(expected.size() == outputVars.size() + 1);
    RC_ASSERT(actual.size() == outputVars.size() + 1);

    for (size_t i = 0; i < expected.size(); ++i) {
      RC_ASSERT(actual.at(i) == expected.at(i));
    }

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(inputVar, varDist(gen));
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
      expected = expectedRow();
      actual = actualRow();
      RC_ASSERT(expected.size() == outputVars.size() + 1);
      RC_ASSERT(actual.size() == outputVars.size() + 1);

      for (size_t i = 0; i < expected.size(); ++i) {
        RC_ASSERT(actual.at(i) == expected.at(i));
      }

    }
    expected = expectedRow(true);
    actual = actualRow(true);
    RC_ASSERT(expected.size() == outputVars.size() + 1);
    RC_ASSERT(actual.size() == outputVars.size() + 1);

    for (size_t i = 0; i < expected.size(); ++i) {
      RC_ASSERT(actual.at(i) == expected.at(i));
    }
  }
}

class MockTable : public Table {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Table::registerVars();
  }
  explicit MockTable(SolverBase& _solver,
                                     std::vector<VarViewId>&& rowViolations,
                                     VarViewId inputVar,
                                     std::vector<std::vector<Int>>&& table,
                                     size_t inputColumn)
      : Table(_solver, std::move(rowViolations),
                              inputVar, std::move(table), inputColumn) {
    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Table::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Table::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Table::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId localId) {
          Table::notifyInputChanged(timestamp, localId);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Table::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

TEST_F(TableTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    std::vector<std::vector<Int>> table(5, std::vector<Int>(numOutputVars + 1));
    for (size_t r = 0; r < table.size(); ++r) {
      for (size_t c = 0; c < table[r].size(); ++c) {
        table[r][c] = -2 + r;
      }
    }
    const VarViewId inputVar = _solver->makeIntVar(0, -100, 100);
    std::vector<VarViewId> outputVars;
    for (size_t i = 0; i < numOutputVars; ++i) {
      outputVars.push_back(_solver->makeIntVar(0, 0, 0));
    }
    const VarViewId queryVarId = outputVars.front();
    testNotifications<MockTable>(
        &_solver->makeInvariant<MockTable>(
            *_solver, std::move(outputVars), inputVar,
            std::move(table), 0),
        {propMode, markingMode, 2, inputVar, 1,
         queryVarId});
  }
}

}  // namespace atlantis::testing
