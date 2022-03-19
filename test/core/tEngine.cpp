
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/elementVar.hpp"
#include "invariants/linear.hpp"
#include "invariants/minSparse.hpp"
#include "views/intOffsetView.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class MockInvariantSimple : public Invariant {
 public:
  bool isRegistered = false;
  bool boundsUpdated = false;

  MockInvariantSimple() : Invariant(NULL_ID) {}

  void registerVars(Engine&) override { isRegistered = true; }
  void updateBounds(Engine&) override { boundsUpdated = true; }

  MOCK_METHOD(void, recompute, (Timestamp, Engine&), (override));

  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine&),
              (override));

  MOCK_METHOD(void, notifyInputChanged, (Timestamp, Engine&, LocalId),
              (override));
  MOCK_METHOD(void, commit, (Timestamp, Engine&), (override));
};

class MockInvariantAdvanced : public Invariant {
 public:
  bool isRegistered = false;
  bool boundsUpdated = false;
  std::vector<VarId> inputs;
  VarId output;

  MockInvariantAdvanced(std::vector<VarId>&& t_inputs, VarId t_output)
      : Invariant(NULL_ID), inputs(std::move(t_inputs)), output(t_output) {
    _modifiedVars.reserve(inputs.size());
  }

  void registerVars(Engine& engine) override {
    assert(_id != NULL_ID);
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine.registerInvariantInput(_id, inputs[i], LocalId(i));
    }
    registerDefinedVariable(engine, output);
    isRegistered = true;
  }

  void updateBounds(Engine&) override { boundsUpdated = true; }

  MOCK_METHOD(void, recompute, (Timestamp, Engine&), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine&),
              (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, Engine&, LocalId),
              (override));
  MOCK_METHOD(void, commit, (Timestamp, Engine&), (override));
};

class MockPlus : public Invariant {
 public:
  bool isRegistered = false;
  VarId a;
  VarId b;
  VarId output;

  MockPlus(VarId t_a, VarId t_b, VarId t_output)
      : Invariant(NULL_ID), a(t_a), b(t_b), output(t_output) {
    _modifiedVars.reserve(2);
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine) {
          updateValue(ts, engine, output,
                      engine.value(ts, a) + engine.value(ts, b));
        });

    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp ts, Engine& engine, LocalId) {
          updateValue(ts, engine, output,
                      engine.value(ts, a) + engine.value(ts, b));
        });
  }

  void registerVars(Engine& engine) override {
    assert(_id != NULL_ID);

    engine.registerInvariantInput(_id, a, LocalId(0));
    engine.registerInvariantInput(_id, b, LocalId(1));
    registerDefinedVariable(engine, output);
    isRegistered = true;
  }

  void updateBounds(Engine& engine) override {
    engine.updateBounds(output, engine.lowerBound(a) + engine.lowerBound(b),
                        engine.upperBound(a) + engine.upperBound(b));
  }

  MOCK_METHOD(void, recompute, (Timestamp, Engine&), (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, Engine&, LocalId),
              (override));
  MOCK_METHOD(void, commit, (Timestamp, Engine&), (override));
};

class EngineTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }

  void propagation(PropagationMode propMode,
                   OutputToInputMarkingMode markingMode) {
    engine->open();
    engine->setPropagationMode(propMode);
    engine->setOutputToInputMarkingMode(markingMode);

    std::vector<std::vector<VarId>> inputs;
    std::vector<VarId> outputs;
    std::vector<MockPlus*> invariants;

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
        inputs[i].emplace_back(engine->makeIntVar(0, 0, 10));
      }
    }
    for (size_t i = 4; i < inputs.size(); ++i) {
      for (VarId output : inputs[i]) {
        outputs.emplace_back(output);
      }
    }
    outputs.emplace_back(engine->makeIntVar(0, 0, 10));

    for (size_t i = 0; i < inputs.size(); ++i) {
      invariants.push_back(&engine->makeInvariant<MockPlus>(
          inputs.at(i).at(0), inputs.at(i).at(1), outputs.at(i)));
    }

    for (MockPlus* invariant : invariants) {
      EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(1);
      EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(1);
    }

    engine->close();

    engine->beginProbe();
    Timestamp timestamp = engine->currentTimestamp();
    VarId modifiedDecisionVariable = inputs[2][1];
    engine->setValue(modifiedDecisionVariable, 1);
    std::vector<size_t> markedInvariants = {2, 5, 6};
    std::vector<size_t> unmarkedInvariants = {0, 1, 3, 4};
    engine->query(outputs.back());

    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      for (size_t i : markedInvariants) {
        EXPECT_CALL(*invariants[i],
                    notifyInputChanged(timestamp, testing::_, LocalId(0)))
            .Times(i == 5 ? 1 : 0);
        EXPECT_CALL(*invariants[i],
                    notifyInputChanged(timestamp, testing::_, LocalId(1)))
            .Times(i == 5 ? 0 : 1);
      }
    } else {
      if (engine->outputToInputMarkingMode() ==
          OutputToInputMarkingMode::NONE) {
        for (size_t i = 0; i < invariants.size(); ++i) {
          VarId a = inputs[i][0];
          VarId b = inputs[i][1];
          EXPECT_CALL(*invariants[i], nextInput(timestamp, testing::_))
              .WillOnce(Return(a))
              .WillOnce(Return(b))
              .WillRepeatedly(Return(NULL_ID));
        }
      } else {
        EXPECT_EQ(engine->modifiedSearchVariables().size(), 1);
        EXPECT_TRUE(
            engine->modifiedSearchVariables().find(modifiedDecisionVariable) !=
            engine->modifiedSearchVariables().end());
        for (size_t i : markedInvariants) {
          EXPECT_CALL(*invariants[i], nextInput(timestamp, testing::_))
              .WillOnce(Return(inputs[i][0]))
              .WillOnce(Return(inputs[i][1]))
              .WillRepeatedly(Return(NULL_ID));
        }
        for (size_t i : unmarkedInvariants) {
          EXPECT_CALL(*invariants[i], nextInput(timestamp, testing::_))
              .Times(0);
        }
      }
      for (size_t i : markedInvariants) {
        EXPECT_CALL(*invariants[i],
                    notifyCurrentInputChanged(timestamp, testing::_))
            .Times(1);
      }
      for (size_t i : unmarkedInvariants) {
        EXPECT_CALL(*invariants[i],
                    notifyCurrentInputChanged(timestamp, testing::_))
            .Times(0);
      }
    }
    engine->endProbe();
  }
};

TEST_F(EngineTest, CreateVariablesAndInvariant) {
  engine->open();

  size_t intVarCount = 10;
  for (size_t value = 0; value < intVarCount; ++value) {
    engine->makeIntVar(value, Int(-100), Int(100));
  }

  // TODO: use some other invariants...
  auto invariant = &engine->makeInvariant<MockInvariantSimple>();

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  ASSERT_TRUE(invariant->isRegistered);

  engine->close();
  EXPECT_EQ(engine->store().numVariables(), intVarCount);
  EXPECT_EQ(engine->store().numInvariants(), size_t(1));
}

TEST_F(EngineTest, ThisTestShouldNotBeHere) {
  // Move this test into a tMinSparse file.
  // I just had to do some quick test and was too lazy to do this propperly.
  engine->open();

  Int intVarCount = 10;
  std::vector<VarId> X;
  for (Int value = 0; value < intVarCount; ++value) {
    X.push_back(engine->makeIntVar(value, Int(-100), Int(100)));
  }

  VarId min = engine->makeIntVar(100, Int(-100), Int(100));
  // TODO: use some other invariants...
  engine->makeInvariant<MinSparse>(X, min);

  engine->close();
  EXPECT_EQ(engine->committedValue(min), 0);
  engine->beginMove();
  engine->setValue(X[0], 5);
  engine->setValue(X[1], 5);
  engine->endMove();

  engine->beginProbe();
  engine->query(min);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(min), 2);

  engine->beginMove();
  engine->setValue(X[3], -1);
  engine->setValue(X[1], 5);
  engine->endMove();

  engine->beginProbe();
  engine->query(min);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(min), -1);

  engine->beginMove();
  engine->setValue(X[0], 1);
  engine->endMove();

  engine->beginProbe();
  engine->query(min);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(min), 1);

  engine->beginMove();
  engine->setValue(X[0], 1);
  engine->endMove();

  engine->beginCommit();
  engine->query(min);
  engine->endCommit();
  EXPECT_EQ(engine->currentValue(min), 1);

  engine->beginMove();
  engine->setValue(X[0], 0);
  engine->endMove();

  engine->beginCommit();
  engine->query(min);
  engine->endCommit();

  EXPECT_EQ(engine->currentValue(min), 0);
}

TEST_F(EngineTest, RecomputeAndCommit) {
  engine->open();

  Int intVarCount = 10;
  for (Int value = 0; value < intVarCount; ++value) {
    engine->makeIntVar(value, -100, 100);
  }

  // TODO: use some other invariants...
  auto invariant = &engine->makeInvariant<MockInvariantSimple>();

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(1);

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(1);

  ASSERT_TRUE(invariant->isRegistered);

  engine->close();

  ASSERT_EQ(engine->store().numVariables(), intVarCount);
  ASSERT_EQ(engine->store().numInvariants(), size_t(1));
}

TEST_F(EngineTest, SimplePropagation) {
  engine->open();

  VarId output = engine->makeIntVar(0, -10, 10);
  VarId a = engine->makeIntVar(1, -10, 10);
  VarId b = engine->makeIntVar(2, -10, 10);
  VarId c = engine->makeIntVar(3, -10, 10);

  auto invariant = &engine->makeInvariant<MockInvariantAdvanced>(
      std::vector<VarId>({a, b, c}), output);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(1);

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(1);

  engine->close();

  engine->beginMove();
  Timestamp moveTimestamp = engine->currentTimestamp();

  engine->setValue(a, -1);
  engine->setValue(b, -2);
  engine->setValue(c, -3);
  engine->endMove();

  if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, nextInput(moveTimestamp, testing::_)).Times(0);
    EXPECT_CALL(*invariant,
                notifyCurrentInputChanged(moveTimestamp, testing::_))
        .Times(0);
  } else {
    EXPECT_CALL(*invariant, nextInput(moveTimestamp, testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant,
                notifyCurrentInputChanged(moveTimestamp, testing::_))
        .Times(3);
  }

  for (size_t id = 0; id < 3; ++id) {
    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, LocalId(id)))
          .Times(1);
    }
  }

  engine->beginProbe();
  engine->query(output);
  engine->endProbe();
}

TEST_F(EngineTest, SimpleCommit) {
  engine->open();

  VarId output = engine->makeIntVar(0, -10, 10);
  VarId a = engine->makeIntVar(1, -10, 10);
  VarId b = engine->makeIntVar(2, -10, 10);
  VarId c = engine->makeIntVar(3, -10, 10);

  auto invariant = &engine->makeInvariant<MockInvariantAdvanced>(
      std::vector<VarId>({a, b, c}), output);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));
  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(0);

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(0);
  } else {
    EXPECT_CALL(*invariant, nextInput(testing::_, testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(3);
  }

  engine->beginMove();
  engine->setValue(a, -1);
  engine->setValue(b, -2);
  engine->setValue(c, -3);
  engine->endMove();

  for (size_t id = 0; id < 3; ++id) {
    if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant,
                  notifyInputChanged(testing::_, testing::_, LocalId(id)))
          .Times(1);
    }
  }

  engine->beginProbe();
  engine->query(output);
  engine->endProbe();

  if (engine->propagationMode() == PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant,
                notifyInputChanged(testing::_, testing::_, LocalId(0)))
        .Times(1);

    EXPECT_CALL(*invariant, nextInput(testing::_, testing::_)).Times(0);

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(0);
  } else if (engine->propagationMode() == PropagationMode::OUTPUT_TO_INPUT) {
    EXPECT_CALL(*invariant, nextInput(testing::_, testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(1);
  }

  engine->beginMove();
  engine->setValue(a, 0);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  EXPECT_EQ(engine->committedValue(a), 0);
  EXPECT_EQ(engine->committedValue(b), 2);
  EXPECT_EQ(engine->committedValue(c), 3);
}

TEST_F(EngineTest, DelayedCommit) {
  engine->open();

  VarId a = engine->makeIntVar(1, -10, 10);
  VarId b = engine->makeIntVar(1, -10, 10);
  VarId c = engine->makeIntVar(1, -10, 10);
  VarId d = engine->makeIntVar(1, -10, 10);
  VarId e = engine->makeIntVar(1, -10, 10);
  VarId f = engine->makeIntVar(1, -10, 10);
  VarId g = engine->makeIntVar(1, -10, 10);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({a, b}), c);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, d}), e);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, f}), g);

  engine->close();
  // 1+1 = c = 2
  // c+1 = e = 3
  // c+1 = g = 3
  EXPECT_EQ(engine->committedValue(c), 2);
  EXPECT_EQ(engine->committedValue(e), 3);
  EXPECT_EQ(engine->committedValue(g), 3);

  engine->beginMove();
  engine->setValue(a, 2);
  engine->endMove();

  engine->beginCommit();
  engine->query(e);
  engine->endCommit();

  EXPECT_EQ(engine->committedValue(e), 4);

  engine->beginMove();
  engine->setValue(d, 0);
  engine->endMove();

  engine->beginCommit();
  engine->query(g);
  engine->endCommit();

  EXPECT_EQ(engine->committedValue(g), 4);
}

TEST_F(EngineTest, TestSimpleDynamicCycleQuery) {
  engine->open();

  VarId x1 = engine->makeIntVar(1, -100, 100);
  VarId x2 = engine->makeIntVar(1, -100, 100);
  VarId x3 = engine->makeIntVar(1, -100, 100);
  VarId base = engine->makeIntVar(1, -10, 10);
  VarId i1 = engine->makeIntVar(1, 1, 4);
  VarId i2 = engine->makeIntVar(2, 1, 4);
  VarId i3 = engine->makeIntVar(3, 1, 4);
  VarId output = engine->makeIntVar(2, -300, 300);

  VarId x1Plus1 = engine->makeIntView<IntOffsetView>(x1, 1);
  VarId x2Plus2 = engine->makeIntView<IntOffsetView>(x2, 2);
  VarId x3Plus3 = engine->makeIntView<IntOffsetView>(x3, 3);

  engine->makeInvariant<ElementVar>(
      i1, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}), x1);
  engine->makeInvariant<ElementVar>(
      i2, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}), x2);
  engine->makeInvariant<ElementVar>(
      i3, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}), x3);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1, 1}),
                                std::vector<VarId>({x1, x2, x3}), output);

  engine->close();

  EXPECT_EQ(engine->committedValue(output), 7);

  {
    engine->beginMove();
    engine->setValue(i1, 4);
    engine->setValue(i2, 1);
    engine->endMove();

    engine->beginProbe();
    engine->query(output);
    engine->endProbe();

    EXPECT_EQ(engine->currentValue(base), 1);
    EXPECT_EQ(engine->currentValue(i1), 4);
    EXPECT_EQ(engine->currentValue(i2), 1);
    EXPECT_EQ(engine->currentValue(i3), 3);
    EXPECT_EQ(engine->currentValue(x1), 6);  // x1 = x3plus3 = x3 + 3
    EXPECT_EQ(engine->currentValue(x2), 1);  // x2 = base = 1
    EXPECT_EQ(engine->currentValue(x3), 3);  // x3 = x2plus2 = x2 + 2

    EXPECT_EQ(engine->currentValue(output), 10);
  }

  {
    engine->beginMove();
    engine->setValue(base, 3);
    engine->endMove();

    engine->beginProbe();
    engine->query(output);
    engine->endProbe();

    EXPECT_EQ(engine->currentValue(base), 3);
    EXPECT_EQ(engine->currentValue(i1), 1);
    EXPECT_EQ(engine->currentValue(i2), 2);
    EXPECT_EQ(engine->currentValue(i3), 3);
    EXPECT_EQ(engine->currentValue(x1), 3);  // x1 = base = 3
    EXPECT_EQ(engine->currentValue(x2), 4);  // x2 = x1plus1 = x1 + 1
    EXPECT_EQ(engine->currentValue(x3), 6);  // x3 = x2plus2 = x2 + 2

    EXPECT_EQ(engine->currentValue(output), 13);
  }

  {
    engine->beginMove();
    engine->setValue(i1, 3);
    engine->setValue(i2, 4);
    engine->setValue(i3, 1);
    engine->setValue(base, 4);
    engine->endMove();

    engine->beginProbe();
    engine->query(output);
    engine->endProbe();

    EXPECT_EQ(engine->currentValue(output), 20);
  }
}

TEST_F(EngineTest, TestSimpleDynamicCycleCommit) {
  engine->open();

  VarId x1 = engine->makeIntVar(1, -100, 100);
  VarId x2 = engine->makeIntVar(1, -100, 100);
  VarId x3 = engine->makeIntVar(1, -100, 100);
  VarId base = engine->makeIntVar(1, -10, 10);
  VarId i1 = engine->makeIntVar(1, 1, 4);
  VarId i2 = engine->makeIntVar(2, 1, 4);
  VarId i3 = engine->makeIntVar(3, 1, 4);
  VarId output = engine->makeIntVar(2, 0, 3);

  VarId viewPlus1 = engine->makeIntView<IntOffsetView>(x1, 1);
  VarId viewPlus2 = engine->makeIntView<IntOffsetView>(x2, 2);
  VarId viewPlus3 = engine->makeIntView<IntOffsetView>(x3, 3);

  engine->makeInvariant<ElementVar>(
      i1, std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}), x1);
  engine->makeInvariant<ElementVar>(
      i2, std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}), x2);
  engine->makeInvariant<ElementVar>(
      i3, std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}), x3);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1, 1}),
                                std::vector<VarId>({x1, x2, x3}), output);

  engine->close();

  EXPECT_EQ(engine->committedValue(output), 7);

  engine->beginMove();
  engine->setValue(i1, 4);
  engine->setValue(i2, 1);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  EXPECT_EQ(engine->currentValue(base), 1);
  EXPECT_EQ(engine->currentValue(i1), 4);
  EXPECT_EQ(engine->currentValue(i2), 1);
  EXPECT_EQ(engine->currentValue(i3), 3);
  EXPECT_EQ(engine->currentValue(x1), 6);  // x1 = x3plus3 = x3 + 3
  EXPECT_EQ(engine->currentValue(x2), 1);  // x2 = base = 1
  EXPECT_EQ(engine->currentValue(x3), 3);  // x3 = x2plus2 = x2 + 2

  EXPECT_EQ(engine->currentValue(output), 10);

  engine->beginMove();
  engine->setValue(base, 3);
  engine->endMove();

  engine->beginProbe();
  engine->query(output);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(base), 3);
  EXPECT_EQ(engine->currentValue(i1), 4);
  EXPECT_EQ(engine->currentValue(i2), 1);
  EXPECT_EQ(engine->currentValue(i3), 3);
  EXPECT_EQ(engine->currentValue(x1), 8);  // x1 = x3plus3 = x3 + 3
  EXPECT_EQ(engine->currentValue(x2), 3);  // x2 = base = 1
  EXPECT_EQ(engine->currentValue(x3), 5);  // x3 = x2plus2 = x2 + 2
  EXPECT_EQ(engine->currentValue(output), 16);

  engine->beginMove();
  engine->setValue(i2, 2);
  engine->setValue(i3, 1);
  engine->setValue(base, 2);
  engine->endMove();

  engine->beginProbe();
  engine->query(output);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(output), 13);

  engine->beginMove();
  engine->setValue(i2, 2);
  engine->setValue(i3, 1);
  engine->setValue(base, 2);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  EXPECT_EQ(engine->currentValue(output), 13);
}

TEST_F(EngineTest, InputToOutputPropagation) {
  propagation(PropagationMode::INPUT_TO_OUTPUT, OutputToInputMarkingMode::NONE);
}

TEST_F(EngineTest, OutputToInputPropagationNone) {
  propagation(PropagationMode::OUTPUT_TO_INPUT, OutputToInputMarkingMode::NONE);
}

TEST_F(EngineTest, OutputToInputPropagationOutputToInputStatic) {
  propagation(PropagationMode::OUTPUT_TO_INPUT,
              OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC);
}

TEST_F(EngineTest, NotificationsOutputToInputInputToOutputExploration) {
  propagation(PropagationMode::OUTPUT_TO_INPUT,
              OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION);
}

}  // namespace
