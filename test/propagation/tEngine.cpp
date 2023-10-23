
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "propagation/invariants/elementVar.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/invariants/minSparse.hpp"
#include "propagation/solver.hpp"
#include "propagation/views/intOffsetView.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

using ::testing::AtLeast;
using ::testing::Return;

class MockInvariantSimple : public Invariant {
 public:
  bool isRegistered = false;
  bool boundsUpdated = false;
  const VarId outputVar;
  const VarId inputVar;

  explicit MockInvariantSimple(SolverBase& solver, VarId t_outputVar,
                               VarId t_inputVar)
      : Invariant(solver), outputVar(t_outputVar), inputVar(t_inputVar) {}

  void registerVars() override {
    _solver.registerInvariantInput(_id, inputVar, LocalId(0));
    registerDefinedVariable(outputVar);
    isRegistered = true;
  }
  void updateBounds(bool) override { boundsUpdated = true; }

  MOCK_METHOD(void, recompute, (Timestamp), (override));

  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));

  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

class MockInvariantAdvanced : public Invariant {
 public:
  bool isRegistered = false;
  bool boundsUpdated = false;
  VarId output;
  std::vector<VarId> inputs;

  explicit MockInvariantAdvanced(SolverBase& solver, VarId t_output,
                                 std::vector<VarId>&& t_inputs)
      : Invariant(solver), output(t_output), inputs(std::move(t_inputs)) {
    _modifiedVars.reserve(inputs.size());
  }

  void registerVars() override {
    assert(_id != NULL_ID);
    for (size_t i = 0; i < inputs.size(); ++i) {
      _solver.registerInvariantInput(_id, inputs[i], LocalId(i));
    }
    registerDefinedVariable(output);
    isRegistered = true;
  }

  void updateBounds(bool) override { boundsUpdated = true; }

  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

class MockSimplePlus : public Invariant {
 public:
  bool isRegistered = false;
  const VarId output, x, y;
  std::vector<InvariantId>* position;

  explicit MockSimplePlus(SolverBase& solver, VarId t_output, VarId t_x,
                          VarId t_y,
                          std::vector<InvariantId>* t_position = nullptr)
      : Invariant(solver),
        output(t_output),
        x(t_x),
        y(t_y),
        position(t_position) {
    _modifiedVars.reserve(2);
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts) {
          updateValue(ts, output, _solver.value(ts, x) + _solver.value(ts, y));
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp ts, LocalId) {
          updateValue(ts, output, _solver.value(ts, x) + _solver.value(ts, y));
        });
  }

  void registerVars() override {
    assert(_id != NULL_ID);

    _solver.registerInvariantInput(_id, x, LocalId(0));
    _solver.registerInvariantInput(_id, y, LocalId(1));
    registerDefinedVariable(output);
    isRegistered = true;
  }

  void updateBounds(bool widenOnly = false) override {
    _solver.updateBounds(output, _solver.lowerBound(x) + _solver.lowerBound(y),
                         _solver.upperBound(x) + _solver.upperBound(y),
                         widenOnly);
    if (position != nullptr) {
      position->emplace_back(_id);
    }
  }

  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};

class SolverTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<Solver> solver;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    solver = std::make_unique<Solver>();
  }

  void propagation(PropagationMode propMode,
                   OutputToInputMarkingMode markingMode) {
    solver->open();
    solver->setPropagationMode(propMode);
    solver->setOutputToInputMarkingMode(markingMode);

    std::vector<std::vector<VarId>> inputs;
    std::vector<VarId> outputs;
    std::vector<MockSimplePlus*> invariants;

    /* +------++------+ +------++------+ +------++------+ +------++------+
     * |inputs||inputs| |inputs||inputs| |inputs||inputs| |inputs||inputs|
     * |[0][0]||[0][1]| |[1][0]||[1][1]| |[2][0]||[2][1]| |[3][0]||[3][1]|
     * +------++------+ +------++------+ +------++------+ +------++------+
     *     \      /         \      /         \      /         \      /
     *  +------------+   +------------+   +------------+   +------------+
     *  |invariant[0]|   |invariant[1]|   |invariant[2]|   |invariant[3]|
     *  +------------+   +------------+   +------------+   +------------+
     *         \               /                 \               /
     *   +------------+ +------------+     +------------+ +------------+
     *   | outputs[0] | | outputs[1] |     | outputs[2] | | outputs[3] |
     *   |inputs[4][0]| |inputs[4][1]|     |inputs[5][0]| |inputs[5][1]|
     *   +------------+ +------------+     +------------+ +------------+
     *           \            /                   \            /
     *           +------------+                   +------------+
     *           |invariant[4]|                   |invariant[5]|
     *           +------------+                   +------------+
     *                   \                            /
     *                  +------------+    +------------+
     *                  | outputs[4] |    | outputs[5] |
     *                  |inputs[6][0]|    |inputs[6][1]|
     *                  +------------+    +------------+
     *                            \          /
     *                           +------------+
     *                           |invariant[6]|
     *                           +------------+
     *                                 |
     *                            +----------+
     *                            |outputs[6]|
     *                            +----------+
     */
    inputs.resize(7);
    for (size_t i = 0; i < inputs.size(); ++i) {
      for (size_t j = 0; j < 2; j++) {
        inputs[i].emplace_back(solver->makeIntVar(0, 0, 10));
      }
    }
    for (size_t i = 4; i < inputs.size(); ++i) {
      for (VarId output : inputs[i]) {
        outputs.emplace_back(output);
      }
    }
    outputs.emplace_back(solver->makeIntVar(0, 0, 10));

    for (size_t i = 0; i < inputs.size(); ++i) {
      invariants.push_back(&solver->makeInvariant<MockSimplePlus>(
          *solver, outputs.at(i), inputs.at(i).at(0), inputs.at(i).at(1)));
    }

    for (MockSimplePlus* invariant : invariants) {
      EXPECT_CALL(*invariant, recompute(::testing::_)).Times(1);
      EXPECT_CALL(*invariant, commit(::testing::_)).Times(1);
    }

    solver->close();

    solver->beginProbe();
    Timestamp timestamp = solver->currentTimestamp();
    VarId modifiedDecisionVariable = inputs[2][1];
    solver->setValue(modifiedDecisionVariable, 1);
    std::vector<size_t> markedInvariants = {2, 5, 6};
    std::vector<size_t> unmarkedInvariants = {0, 1, 3, 4};
    solver->query(outputs.back());

    if (solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      for (size_t i : markedInvariants) {
        EXPECT_CALL(*invariants[i], notifyInputChanged(timestamp, LocalId(0)))
            .Times(i == 5 ? 1 : 0);
        EXPECT_CALL(*invariants[i], notifyInputChanged(timestamp, LocalId(1)))
            .Times(i == 5 ? 0 : 1);
      }
    } else {
      if (solver->outputToInputMarkingMode() ==
          OutputToInputMarkingMode::NONE) {
        for (size_t i = 0; i < invariants.size(); ++i) {
          VarId a = inputs[i][0];
          VarId b = inputs[i][1];
          EXPECT_CALL(*invariants[i], nextInput(timestamp))
              .WillOnce(Return(a))
              .WillOnce(Return(b))
              .WillRepeatedly(Return(NULL_ID));
        }
      } else {
        EXPECT_EQ(solver->modifiedSearchVariables().size(), 1);
        EXPECT_TRUE(solver->modifiedSearchVariables().contains(
            modifiedDecisionVariable));
        for (size_t i : markedInvariants) {
          EXPECT_CALL(*invariants[i], nextInput(timestamp))
              .WillOnce(Return(inputs[i][0]))
              .WillOnce(Return(inputs[i][1]))
              .WillRepeatedly(Return(NULL_ID));
        }
        for (size_t i : unmarkedInvariants) {
          EXPECT_CALL(*invariants[i], nextInput(timestamp)).Times(0);
        }
      }
      for (size_t i : markedInvariants) {
        EXPECT_CALL(*invariants[i], notifyCurrentInputChanged(timestamp))
            .Times(1);
      }
      for (size_t i : unmarkedInvariants) {
        EXPECT_CALL(*invariants[i], notifyCurrentInputChanged(timestamp))
            .Times(0);
      }
    }
    solver->endProbe();
  }
};

TEST_F(SolverTest, CreateVariablesAndInvariant) {
  solver->open();

  const VarId inputVar = solver->makeIntVar(0, Int(-100), Int(100));
  const VarId outputVar = solver->makeIntVar(0, Int(-100), Int(100));

  // TODO: use some other invariants...
  auto invariant =
      &solver->makeInvariant<MockInvariantSimple>(*solver, outputVar, inputVar);

  EXPECT_CALL(*invariant, recompute(::testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(::testing::_)).Times(AtLeast(1));

  ASSERT_TRUE(invariant->isRegistered);

  solver->close();
  EXPECT_EQ(solver->store().numVariables(), 2);
  EXPECT_EQ(solver->store().numInvariants(), size_t(1));
}

TEST_F(SolverTest, ThisTestShouldNotBeHere) {
  // Move this test into a tMinSparse file.
  // I just had to do some quick test and was too lazy to do this propperly.
  solver->open();

  Int intVarCount = 10;
  std::vector<VarId> X;
  for (Int value = 0; value < intVarCount; ++value) {
    X.push_back(solver->makeIntVar(value, Int(-100), Int(100)));
  }

  VarId min = solver->makeIntVar(100, Int(-100), Int(100));
  // TODO: use some other invariants...
  solver->makeInvariant<MinSparse>(*solver, min, X);

  solver->close();
  EXPECT_EQ(solver->committedValue(min), 0);
  solver->beginMove();
  solver->setValue(X[0], 5);
  solver->setValue(X[1], 5);
  solver->endMove();

  solver->beginProbe();
  solver->query(min);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(min), 2);

  solver->beginMove();
  solver->setValue(X[3], -1);
  solver->setValue(X[1], 5);
  solver->endMove();

  solver->beginProbe();
  solver->query(min);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(min), -1);

  solver->beginMove();
  solver->setValue(X[0], 1);
  solver->endMove();

  solver->beginProbe();
  solver->query(min);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(min), 1);

  solver->beginMove();
  solver->setValue(X[0], 1);
  solver->endMove();

  solver->beginCommit();
  solver->query(min);
  solver->endCommit();
  EXPECT_EQ(solver->currentValue(min), 1);

  solver->beginMove();
  solver->setValue(X[0], 0);
  solver->endMove();

  solver->beginCommit();
  solver->query(min);
  solver->endCommit();

  EXPECT_EQ(solver->currentValue(min), 0);
}

TEST_F(SolverTest, RecomputeAndCommit) {
  solver->open();

  const VarId inputVar = solver->makeIntVar(0, -100, 100);
  const VarId outputVar = solver->makeIntVar(0, -100, 100);

  // TODO: use some other invariants...
  auto invariant =
      &solver->makeInvariant<MockInvariantSimple>(*solver, outputVar, inputVar);

  EXPECT_CALL(*invariant, recompute(::testing::_)).Times(1);

  EXPECT_CALL(*invariant, commit(::testing::_)).Times(1);

  ASSERT_TRUE(invariant->isRegistered);

  solver->close();

  ASSERT_EQ(solver->store().numVariables(), 2);
  ASSERT_EQ(solver->store().numInvariants(), size_t(1));
}

TEST_F(SolverTest, SimplePropagation) {
  solver->open();

  VarId output = solver->makeIntVar(0, -10, 10);
  VarId a = solver->makeIntVar(1, -10, 10);
  VarId b = solver->makeIntVar(2, -10, 10);
  VarId c = solver->makeIntVar(3, -10, 10);

  auto invariant = &solver->makeInvariant<MockInvariantAdvanced>(
      *solver, output, std::vector<VarId>({a, b, c}));

  EXPECT_CALL(*invariant, recompute(::testing::_)).Times(1);

  EXPECT_CALL(*invariant, commit(::testing::_)).Times(1);

  solver->close();

  solver->beginMove();
  Timestamp moveTimestamp = solver->currentTimestamp();

  solver->setValue(a, -1);
  solver->setValue(b, -2);
  solver->setValue(c, -3);
  solver->endMove();

  if (solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, nextInput(moveTimestamp)).Times(0);
    EXPECT_CALL(*invariant, notifyCurrentInputChanged(moveTimestamp)).Times(0);
  } else {
    EXPECT_CALL(*invariant, nextInput(moveTimestamp))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(moveTimestamp)).Times(3);
  }

  for (size_t id = 0; id < 3; ++id) {
    if (solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, notifyInputChanged(::testing::_, LocalId(id)))
          .Times(1);
    }
  }

  solver->beginProbe();
  solver->query(output);
  solver->endProbe();
}

TEST_F(SolverTest, SimpleCommit) {
  solver->open();

  VarId output = solver->makeIntVar(0, -10, 10);
  VarId a = solver->makeIntVar(1, -10, 10);
  VarId b = solver->makeIntVar(2, -10, 10);
  VarId c = solver->makeIntVar(3, -10, 10);

  auto invariant = &solver->makeInvariant<MockInvariantAdvanced>(
      *solver, output, std::vector<VarId>({a, b, c}));

  EXPECT_CALL(*invariant, recompute(::testing::_)).Times(AtLeast(1));
  EXPECT_CALL(*invariant, commit(::testing::_)).Times(AtLeast(1));

  solver->close();

  if (solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, nextInput(::testing::_)).Times(0);

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(::testing::_)).Times(0);
  } else {
    EXPECT_CALL(*invariant, nextInput(::testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(::testing::_)).Times(3);
  }

  solver->beginMove();
  solver->setValue(a, -1);
  solver->setValue(b, -2);
  solver->setValue(c, -3);
  solver->endMove();

  for (size_t id = 0; id < 3; ++id) {
    if (solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant, notifyInputChanged(::testing::_, LocalId(id)))
          .Times(1);
    }
  }

  solver->beginProbe();
  solver->query(output);
  solver->endProbe();

  if (solver->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, notifyInputChanged(::testing::_, LocalId(0)))
        .Times(1);

    EXPECT_CALL(*invariant, nextInput(::testing::_)).Times(0);

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(::testing::_)).Times(0);
  } else if (solver->propagationMode() == PropagationMode::OUTPUT_TO_INPUT) {
    EXPECT_CALL(*invariant, nextInput(::testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(::testing::_)).Times(1);
  }

  solver->beginMove();
  solver->setValue(a, 0);
  solver->endMove();

  solver->beginCommit();
  solver->query(output);
  solver->endCommit();

  EXPECT_EQ(solver->committedValue(a), 0);
  EXPECT_EQ(solver->committedValue(b), 2);
  EXPECT_EQ(solver->committedValue(c), 3);
}

TEST_F(SolverTest, DelayedCommit) {
  solver->open();

  VarId a = solver->makeIntVar(1, -10, 10);
  VarId b = solver->makeIntVar(1, -10, 10);
  VarId c = solver->makeIntVar(1, -10, 10);
  VarId d = solver->makeIntVar(1, -10, 10);
  VarId e = solver->makeIntVar(1, -10, 10);
  VarId f = solver->makeIntVar(1, -10, 10);
  VarId g = solver->makeIntVar(1, -10, 10);

  solver->makeInvariant<Linear>(*solver, c, std::vector<Int>({1, 1}),
                                std::vector<VarId>({a, b}));

  solver->makeInvariant<Linear>(*solver, e, std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, d}));

  solver->makeInvariant<Linear>(*solver, g, std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, f}));

  solver->close();
  // 1+1 = c = 2
  // c+1 = e = 3
  // c+1 = g = 3
  EXPECT_EQ(solver->committedValue(c), 2);
  EXPECT_EQ(solver->committedValue(e), 3);
  EXPECT_EQ(solver->committedValue(g), 3);

  solver->beginMove();
  solver->setValue(a, 2);
  solver->endMove();

  solver->beginCommit();
  solver->query(e);
  solver->endCommit();

  EXPECT_EQ(solver->committedValue(e), 4);

  solver->beginMove();
  solver->setValue(d, 0);
  solver->endMove();

  solver->beginCommit();
  solver->query(g);
  solver->endCommit();

  EXPECT_EQ(solver->committedValue(g), 4);
}

TEST_F(SolverTest, TestSimpleDynamicCycleQuery) {
  solver->open();

  VarId x1 = solver->makeIntVar(1, -100, 100);
  VarId x2 = solver->makeIntVar(1, -100, 100);
  VarId x3 = solver->makeIntVar(1, -100, 100);
  VarId base = solver->makeIntVar(1, -10, 10);
  VarId i1 = solver->makeIntVar(1, 1, 4);
  VarId i2 = solver->makeIntVar(2, 1, 4);
  VarId i3 = solver->makeIntVar(3, 1, 4);
  VarId output = solver->makeIntVar(2, -300, 300);

  VarId x1Plus1 = solver->makeIntView<IntOffsetView>(*solver, x1, 1);
  VarId x2Plus2 = solver->makeIntView<IntOffsetView>(*solver, x2, 2);
  VarId x3Plus3 = solver->makeIntView<IntOffsetView>(*solver, x3, 3);

  solver->makeInvariant<ElementVar>(
      *solver, x1, i1, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}));
  solver->makeInvariant<ElementVar>(
      *solver, x2, i2, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}));
  solver->makeInvariant<ElementVar>(
      *solver, x3, i3, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}));

  solver->makeInvariant<Linear>(*solver, output, std::vector<Int>({1, 1, 1}),
                                std::vector<VarId>({x1, x2, x3}));

  solver->close();

  EXPECT_EQ(solver->committedValue(output), 7);

  {
    solver->beginMove();
    solver->setValue(i1, 4);
    solver->setValue(i2, 1);
    solver->endMove();

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();

    EXPECT_EQ(solver->currentValue(base), 1);
    EXPECT_EQ(solver->currentValue(i1), 4);
    EXPECT_EQ(solver->currentValue(i2), 1);
    EXPECT_EQ(solver->currentValue(i3), 3);
    EXPECT_EQ(solver->currentValue(x1), 6);  // x1 = x3plus3 = x3 + 3
    EXPECT_EQ(solver->currentValue(x2), 1);  // x2 = base = 1
    EXPECT_EQ(solver->currentValue(x3), 3);  // x3 = x2plus2 = x2 + 2

    EXPECT_EQ(solver->currentValue(output), 10);
  }

  {
    solver->beginMove();
    solver->setValue(base, 3);
    solver->endMove();

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();

    EXPECT_EQ(solver->currentValue(base), 3);
    EXPECT_EQ(solver->currentValue(i1), 1);
    EXPECT_EQ(solver->currentValue(i2), 2);
    EXPECT_EQ(solver->currentValue(i3), 3);
    EXPECT_EQ(solver->currentValue(x1), 3);  // x1 = base = 3
    EXPECT_EQ(solver->currentValue(x2), 4);  // x2 = x1plus1 = x1 + 1
    EXPECT_EQ(solver->currentValue(x3), 6);  // x3 = x2plus2 = x2 + 2

    EXPECT_EQ(solver->currentValue(output), 13);
  }

  {
    solver->beginMove();
    solver->setValue(i1, 3);
    solver->setValue(i2, 4);
    solver->setValue(i3, 1);
    solver->setValue(base, 4);
    solver->endMove();

    solver->beginProbe();
    solver->query(output);
    solver->endProbe();

    EXPECT_EQ(solver->currentValue(output), 20);
  }
}

TEST_F(SolverTest, TestSimpleDynamicCycleCommit) {
  solver->open();

  VarId x1 = solver->makeIntVar(1, -100, 100);
  VarId x2 = solver->makeIntVar(1, -100, 100);
  VarId x3 = solver->makeIntVar(1, -100, 100);
  VarId base = solver->makeIntVar(1, -10, 10);
  VarId i1 = solver->makeIntVar(1, 1, 4);
  VarId i2 = solver->makeIntVar(2, 1, 4);
  VarId i3 = solver->makeIntVar(3, 1, 4);
  VarId output = solver->makeIntVar(2, 0, 3);

  VarId viewPlus1 = solver->makeIntView<IntOffsetView>(*solver, x1, 1);
  VarId viewPlus2 = solver->makeIntView<IntOffsetView>(*solver, x2, 2);
  VarId viewPlus3 = solver->makeIntView<IntOffsetView>(*solver, x3, 3);

  solver->makeInvariant<ElementVar>(
      *solver, x1, i1,
      std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}));
  solver->makeInvariant<ElementVar>(
      *solver, x2, i2,
      std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}));
  solver->makeInvariant<ElementVar>(
      *solver, x3, i3,
      std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}));

  solver->makeInvariant<Linear>(*solver, output, std::vector<Int>({1, 1, 1}),
                                std::vector<VarId>({x1, x2, x3}));

  solver->close();

  EXPECT_EQ(solver->committedValue(output), 7);

  solver->beginMove();
  solver->setValue(i1, 4);
  solver->setValue(i2, 1);
  solver->endMove();

  solver->beginCommit();
  solver->query(output);
  solver->endCommit();

  EXPECT_EQ(solver->currentValue(base), 1);
  EXPECT_EQ(solver->currentValue(i1), 4);
  EXPECT_EQ(solver->currentValue(i2), 1);
  EXPECT_EQ(solver->currentValue(i3), 3);
  EXPECT_EQ(solver->currentValue(x1), 6);  // x1 = x3plus3 = x3 + 3
  EXPECT_EQ(solver->currentValue(x2), 1);  // x2 = base = 1
  EXPECT_EQ(solver->currentValue(x3), 3);  // x3 = x2plus2 = x2 + 2

  EXPECT_EQ(solver->currentValue(output), 10);

  solver->beginMove();
  solver->setValue(base, 3);
  solver->endMove();

  solver->beginProbe();
  solver->query(output);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(base), 3);
  EXPECT_EQ(solver->currentValue(i1), 4);
  EXPECT_EQ(solver->currentValue(i2), 1);
  EXPECT_EQ(solver->currentValue(i3), 3);
  EXPECT_EQ(solver->currentValue(x1), 8);  // x1 = x3plus3 = x3 + 3
  EXPECT_EQ(solver->currentValue(x2), 3);  // x2 = base = 1
  EXPECT_EQ(solver->currentValue(x3), 5);  // x3 = x2plus2 = x2 + 2
  EXPECT_EQ(solver->currentValue(output), 16);

  solver->beginMove();
  solver->setValue(i2, 2);
  solver->setValue(i3, 1);
  solver->setValue(base, 2);
  solver->endMove();

  solver->beginProbe();
  solver->query(output);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(output), 13);

  solver->beginMove();
  solver->setValue(i2, 2);
  solver->setValue(i3, 1);
  solver->setValue(base, 2);
  solver->endMove();

  solver->beginCommit();
  solver->query(output);
  solver->endCommit();

  EXPECT_EQ(solver->currentValue(output), 13);
}
TEST_F(SolverTest, ComputeBounds) {
  solver->open();

  std::vector<std::vector<VarId>> inputs;
  std::vector<VarId> outputs;
  std::vector<MockSimplePlus*> invariants;
  std::vector<InvariantId> position;

  /* +------++------+ +------++------+ +------++------+ +------++------+
   * |inputs||inputs| |inputs||inputs| |inputs||inputs| |inputs||inputs|
   * |[0][0]||[0][1]| |[1][0]||[1][1]| |[2][0]||[2][1]| |[3][0]||[3][1]|
   * +------++------+ +------++------+ +------++------+ +------++------+
   *     \      /         \      /         \      /         \      /
   *  +------------+   +------------+   +------------+   +------------+
   *  |invariant[0]|   |invariant[1]|   |invariant[2]|   |invariant[3]|
   *  +------------+   +------------+   +------------+   +------------+
   *         \               /                 \               /
   *   +------------+ +------------+     +------------+ +------------+
   *   | outputs[0] | | outputs[1] |     | outputs[2] | | outputs[3] |
   *   |inputs[4][0]| |inputs[4][1]|     |inputs[5][0]| |inputs[5][1]|
   *   +------------+ +------------+     +------------+ +------------+
   *           \            /                   \            /
   *           +------------+                   +------------+
   *           |invariant[4]|                   |invariant[5]|
   *           +------------+                   +------------+
   *                   \                            /
   *                  +------------+    +------------+
   *                  | outputs[4] |    | outputs[5] |
   *                  |inputs[6][0]|    |inputs[6][1]|
   *                  +------------+    +------------+
   *                            \          /
   *                           +------------+
   *                           |invariant[6]|
   *                           +------------+
   *                                 |
   *                            +----------+
   *                            |outputs[6]|
   *                            +----------+
   */
  inputs.resize(7);
  for (size_t i = 0; i < inputs.size(); ++i) {
    const Int lb = i < 4 ? 5 : 0;
    const Int ub = i < 4 ? 10 : 0;
    for (size_t j = 0; j < 2; j++) {
      inputs[i].emplace_back(solver->makeIntVar(lb, lb, ub));
    }
  }
  for (size_t i = 4; i < inputs.size(); ++i) {
    for (VarId output : inputs[i]) {
      outputs.emplace_back(output);
    }
  }
  outputs.emplace_back(solver->makeIntVar(0, 0, 0));

  for (size_t i = 0; i < inputs.size(); ++i) {
    invariants.push_back(&solver->makeInvariant<MockSimplePlus>(
        *solver, outputs.at(i), inputs.at(i).at(0), inputs.at(i).at(1),
        &position));
  }

  std::vector<std::pair<Int, Int>> expectedBounds = {
      std::pair<Int, Int>{10, 20}, std::pair<Int, Int>{10, 20},
      std::pair<Int, Int>{10, 20}, std::pair<Int, Int>{10, 20},
      std::pair<Int, Int>{20, 40}, std::pair<Int, Int>{20, 40},
      std::pair<Int, Int>{40, 80}};

  EXPECT_EQ(invariants.size(), position.size());

  for (size_t i = 0; i < invariants.size(); ++i) {
    EXPECT_EQ(invariants.at(i)->id(), position.at(i));
  }

  for (size_t v = 0; v < outputs.size(); ++v) {
    EXPECT_EQ(solver->lowerBound(outputs.at(v)), expectedBounds.at(v).first);
    EXPECT_EQ(solver->upperBound(outputs.at(v)), expectedBounds.at(v).second);
  }

  position.clear();

  solver->computeBounds();

  EXPECT_EQ(invariants.size(), position.size());

  for (size_t i = 0; i < invariants.size(); ++i) {
    EXPECT_EQ(invariants.at(i)->id(), position.at(i));
  }

  for (size_t v = 0; v < outputs.size(); ++v) {
    EXPECT_EQ(solver->lowerBound(outputs.at(v)), expectedBounds.at(v).first);
    EXPECT_EQ(solver->upperBound(outputs.at(v)), expectedBounds.at(v).second);
  }
}

TEST_F(SolverTest, ComputeBoundsCycle) {
  solver->open();

  std::vector<InvariantId> position;
  std::vector<MockSimplePlus*> invariants;

  const Int lb = 5;
  const Int ub = 10;

  std::vector<std::vector<VarId>> inputs{
      std::vector<VarId>{solver->makeIntVar(lb, lb, ub),
                         solver->makeIntVar(0, 0, 0)},
      std::vector<VarId>{solver->makeIntVar(lb, lb, ub),
                         solver->makeIntVar(0, 0, 0)}};

  std::vector<VarId> outputs{inputs.at(1).at(1), inputs.at(0).at(1)};

  for (size_t i = 0; i < inputs.size(); ++i) {
    invariants.push_back(&solver->makeInvariant<MockSimplePlus>(
        *solver, outputs.at(i), inputs.at(i).at(0), inputs.at(i).at(1),
        &position));
  }

  EXPECT_EQ(invariants.size(), position.size());

  for (size_t i = 0; i < invariants.size(); ++i) {
    EXPECT_EQ(invariants.at(i)->id(), position.at(i));
  }

  EXPECT_EQ(solver->lowerBound(outputs.at(0)), 5);
  EXPECT_EQ(solver->upperBound(outputs.at(0)), 10);
  EXPECT_EQ(solver->lowerBound(outputs.at(1)), 10);
  EXPECT_EQ(solver->upperBound(outputs.at(1)), 20);

  position.clear();

  solver->computeBounds();

  EXPECT_EQ(invariants.size(), position.size() - 1);

  for (size_t i = 0; i < invariants.size(); ++i) {
    EXPECT_EQ(invariants.at(i)->id(), position.at(i));
  }
  EXPECT_EQ(invariants.front()->id(), position.back());

  EXPECT_EQ(solver->lowerBound(outputs.at(0)), 5);
  EXPECT_EQ(solver->upperBound(outputs.at(0)), 50);

  EXPECT_EQ(solver->lowerBound(outputs.at(1)), 10);
  EXPECT_EQ(solver->upperBound(outputs.at(1)), 40);
}

TEST_F(SolverTest, InputToOutputPropagation) {
  propagation(PropagationMode::INPUT_TO_OUTPUT, OutputToInputMarkingMode::NONE);
}

TEST_F(SolverTest, OutputToInputPropagationNone) {
  propagation(PropagationMode::OUTPUT_TO_INPUT, OutputToInputMarkingMode::NONE);
}

TEST_F(SolverTest, OutputToInputPropagationOutputToInputStatic) {
  propagation(PropagationMode::OUTPUT_TO_INPUT,
              OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(SolverTest, NotificationsOutputToInputInputToOutputExploration) {
  propagation(PropagationMode::OUTPUT_TO_INPUT,
              OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace atlantis::testing
