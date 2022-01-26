
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

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
  bool _initialized = false;

  MockInvariantSimple() : Invariant(NULL_ID) {}

  void init(Timestamp, Engine&) override { _initialized = true; }

  MOCK_METHOD(void, recompute, (Timestamp, Engine&), (override));

  MOCK_METHOD(VarId, getNextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine&),
              (override));

  MOCK_METHOD(void, notifyIntChanged, (Timestamp, Engine&, LocalId),
              (override));
  MOCK_METHOD(void, commit, (Timestamp, Engine&), (override));
};

class MockInvariantAdvanced : public Invariant {
 public:
  bool _initialized = false;
  std::vector<VarId> inputs;
  VarId output;

  MockInvariantAdvanced(std::vector<VarId>&& t_inputs, VarId t_output)
      : Invariant(NULL_ID), inputs(std::move(t_inputs)), output(t_output) {
    _modifiedVars.reserve(inputs.size());
  }

  void init(Timestamp, Engine& engine) override {
    assert(_id != NULL_ID);

    registerDefinedVariable(engine, output);
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine.registerInvariantInput(_id, inputs[i], LocalId(i));
    }
    _initialized = true;
  }

  MOCK_METHOD(void, recompute, (Timestamp, Engine&), (override));
  MOCK_METHOD(VarId, getNextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine&),
              (override));
  MOCK_METHOD(void, notifyIntChanged, (Timestamp, Engine&, LocalId),
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
};

TEST_F(EngineTest, CreateVariablesAndInvariant) {
  engine->open();

  size_t intVarCount = 10;
  for (size_t value = 0; value < intVarCount; ++value) {
    engine->makeIntVar(value, Int(-100), Int(100));
  }

  // TODO: use some other invariants...
  auto invariant = engine->makeInvariant<MockInvariantSimple>();

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  ASSERT_TRUE(invariant->_initialized);

  engine->close();
  EXPECT_EQ(engine->getStore().getNumVariables(), intVarCount);
  EXPECT_EQ(engine->getStore().getNumInvariants(), size_t(1));
}

TEST_F(EngineTest, ThisTestShouldNotBeHere) {
  // Move this test into a tMinSparse file.
  // I just had to do some quick test and was too lazy to do this propperly.
  engine->open();

  size_t intVarCount = 10;
  std::vector<VarId> X;
  for (size_t value = 0; value < intVarCount; ++value) {
    X.push_back(engine->makeIntVar(value, Int(-100), Int(100)));
  }

  VarId min = engine->makeIntVar(100, Int(-100), Int(100));
  // TODO: use some other invariants...
  auto invariant = engine->makeInvariant<MinSparse>(X, min);

  engine->close();
  EXPECT_EQ(engine->getCommittedValue(min), 0);
  engine->beginMove();
  engine->setValue(X[0], 5);
  engine->setValue(X[1], 5);
  engine->endMove();

  engine->beginQuery();
  engine->query(min);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(min), 2);

  engine->beginMove();
  engine->setValue(X[3], -1);
  engine->setValue(X[1], 5);
  engine->endMove();

  engine->beginQuery();
  engine->query(min);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(min), -1);

  engine->beginMove();
  engine->setValue(X[0], 1);
  engine->endMove();

  engine->beginQuery();
  engine->query(min);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(min), 1);

  engine->beginMove();
  engine->setValue(X[0], 1);
  engine->endMove();

  engine->beginCommit();
  engine->query(min);
  engine->endCommit();
  EXPECT_EQ(engine->getNewValue(min), 1);

  engine->beginMove();
  engine->setValue(X[0], 0);
  engine->endMove();

  engine->beginCommit();
  engine->query(min);
  engine->endCommit();

  EXPECT_EQ(engine->getNewValue(min), 0);
}

TEST_F(EngineTest, RecomputeAndCommit) {
  engine->open();

  size_t intVarCount = 10;
  for (size_t value = 0; value < intVarCount; ++value) {
    engine->makeIntVar(value, -100, 100);
  }

  // TODO: use some other invariants...
  auto invariant = engine->makeInvariant<MockInvariantSimple>();

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(1);

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(1);

  ASSERT_TRUE(invariant->_initialized);

  engine->close();

  ASSERT_EQ(engine->getStore().getNumVariables(), intVarCount);
  ASSERT_EQ(engine->getStore().getNumInvariants(), size_t(1));
}

TEST_F(EngineTest, SimplePropagation) {
  engine->open();

  VarId output = engine->makeIntVar(0, -10, 10);
  VarId a = engine->makeIntVar(1, -10, 10);
  VarId b = engine->makeIntVar(2, -10, 10);
  VarId c = engine->makeIntVar(3, -10, 10);

  auto invariant = engine->makeInvariant<MockInvariantAdvanced>(
      std::vector<VarId>({a, b, c}), output);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(1);

  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(1);

  engine->close();

  engine->beginMove();
  Timestamp moveTimestamp = engine->getCurrentTimestamp();

  engine->setValue(a, -1);
  engine->setValue(b, -2);
  engine->setValue(c, -3);
  engine->endMove();

  if (engine->propagationMode ==
      PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, getNextInput(moveTimestamp, testing::_)).Times(0);

    EXPECT_CALL(*invariant,
                notifyCurrentInputChanged(moveTimestamp, testing::_))
        .Times(0);
  } else {
    EXPECT_CALL(*invariant, getNextInput(moveTimestamp, testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant,
                notifyCurrentInputChanged(moveTimestamp, testing::_))
        .Times(3);
  }

  for (size_t id = 0; id < 3; ++id) {
    if (engine->propagationMode ==
        PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, LocalId(id)))
          .Times(1);
    }
  }

  engine->beginQuery();
  engine->query(output);
  engine->endQuery();
}

TEST_F(EngineTest, SimpleCommit) {
  engine->open();

  VarId output = engine->makeIntVar(0, -10, 10);
  VarId a = engine->makeIntVar(1, -10, 10);
  VarId b = engine->makeIntVar(2, -10, 10);
  VarId c = engine->makeIntVar(3, -10, 10);

  auto invariant = engine->makeInvariant<MockInvariantAdvanced>(
      std::vector<VarId>({a, b, c}), output);

  EXPECT_CALL(*invariant, recompute(testing::_, testing::_)).Times(AtLeast(1));
  EXPECT_CALL(*invariant, commit(testing::_, testing::_)).Times(AtLeast(1));

  engine->close();

  if (engine->propagationMode ==
      PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_)).Times(0);

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(0);
  } else {
    EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_))
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
    if (engine->propagationMode ==
        PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
      EXPECT_CALL(*invariant,
                  notifyIntChanged(testing::_, testing::_, LocalId(id)))
          .Times(1);
    }
  }

  engine->beginQuery();
  engine->query(output);
  engine->endQuery();

  if (engine->propagationMode ==
      PropagationEngine::PropagationMode::INPUT_TO_OUTPUT) {
    EXPECT_CALL(*invariant,
                notifyIntChanged(testing::_, testing::_, LocalId(0)))
        .Times(1);

    EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_)).Times(0);

    EXPECT_CALL(*invariant, notifyCurrentInputChanged(testing::_, testing::_))
        .Times(0);
  } else if (engine->propagationMode ==
             PropagationEngine::PropagationMode::OUTPUT_TO_INPUT) {
    EXPECT_CALL(*invariant, getNextInput(testing::_, testing::_))
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

  EXPECT_EQ(engine->getCommittedValue(a), 0);
  EXPECT_EQ(engine->getCommittedValue(b), 2);
  EXPECT_EQ(engine->getCommittedValue(c), 3);
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
  EXPECT_EQ(engine->getCommittedValue(c), 2);
  EXPECT_EQ(engine->getCommittedValue(e), 3);
  EXPECT_EQ(engine->getCommittedValue(g), 3);

  engine->beginMove();
  engine->setValue(a, 2);
  engine->endMove();

  engine->beginCommit();
  engine->query(e);
  engine->endCommit();

  EXPECT_EQ(engine->getCommittedValue(e), 4);

  engine->beginMove();
  engine->setValue(d, 0);
  engine->endMove();

  engine->beginCommit();
  engine->query(g);
  engine->endCommit();

  EXPECT_EQ(engine->getCommittedValue(g), 4);
}

TEST_F(EngineTest, TestSimpleDynamicCycleQuery) {
  engine->open();

  VarId x1 = engine->makeIntVar(1, -100, 100);
  VarId x2 = engine->makeIntVar(1, -100, 100);
  VarId x3 = engine->makeIntVar(1, -100, 100);
  VarId base = engine->makeIntVar(1, -10, 10);
  VarId i1 = engine->makeIntVar(0, 0, 3);
  VarId i2 = engine->makeIntVar(1, 0, 3);
  VarId i3 = engine->makeIntVar(2, 0, 3);
  VarId output = engine->makeIntVar(2, -300, 300);

  VarId x1Plus1 = engine->makeIntView<IntOffsetView>(x1, 1)->getId();
  VarId x2Plus2 = engine->makeIntView<IntOffsetView>(x2, 2)->getId();
  VarId x3Plus3 = engine->makeIntView<IntOffsetView>(x3, 3)->getId();

  engine->makeInvariant<ElementVar>(
      i1, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}), x1);
  engine->makeInvariant<ElementVar>(
      i2, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}), x2);
  engine->makeInvariant<ElementVar>(
      i3, std::vector<VarId>({base, x1Plus1, x2Plus2, x3Plus3}), x3);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1, 1}),
                                std::vector<VarId>({x1, x2, x3}), output);

  engine->close();

  EXPECT_EQ(engine->getCommittedValue(output), 7);

  {
    engine->beginMove();
    engine->setValue(i1, 3);
    engine->setValue(i2, 0);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();

    EXPECT_EQ(engine->getNewValue(base), 1);
    EXPECT_EQ(engine->getNewValue(i1), 3);
    EXPECT_EQ(engine->getNewValue(i2), 0);
    EXPECT_EQ(engine->getNewValue(i3), 2);
    EXPECT_EQ(engine->getNewValue(x1), 6);  // x1 = x3plus3 = x3 + 3
    EXPECT_EQ(engine->getNewValue(x2), 1);  // x2 = base = 1
    EXPECT_EQ(engine->getNewValue(x3), 3);  // x3 = x2plus2 = x2 + 2

    EXPECT_EQ(engine->getNewValue(output), 10);
  }

  {
    engine->beginMove();
    engine->setValue(base, 3);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();

    EXPECT_EQ(engine->getNewValue(base), 3);
    EXPECT_EQ(engine->getNewValue(i1), 0);
    EXPECT_EQ(engine->getNewValue(i2), 1);
    EXPECT_EQ(engine->getNewValue(i3), 2);
    EXPECT_EQ(engine->getNewValue(x1), 3);  // x1 = base = 3
    EXPECT_EQ(engine->getNewValue(x2), 4);  // x2 = x1plus1 = x1 + 1
    EXPECT_EQ(engine->getNewValue(x3), 6);  // x3 = x2plus2 = x2 + 2

    EXPECT_EQ(engine->getNewValue(output), 13);
  }

  {
    engine->beginMove();
    engine->setValue(i1, 2);
    engine->setValue(i2, 3);
    engine->setValue(i3, 0);
    engine->setValue(base, 4);
    engine->endMove();

    engine->beginQuery();
    engine->query(output);
    engine->endQuery();

    EXPECT_EQ(engine->getNewValue(output), 20);
  }
}

TEST_F(EngineTest, TestSimpleDynamicCycleCommit) {
  engine->open();

  VarId x1 = engine->makeIntVar(1, -100, 100);
  VarId x2 = engine->makeIntVar(1, -100, 100);
  VarId x3 = engine->makeIntVar(1, -100, 100);
  VarId base = engine->makeIntVar(1, -10, 10);
  VarId i1 = engine->makeIntVar(0, 0, 3);
  VarId i2 = engine->makeIntVar(1, 0, 3);
  VarId i3 = engine->makeIntVar(2, 0, 3);
  VarId output = engine->makeIntVar(2, 0, 3);

  VarId viewPlus1 = engine->makeIntView<IntOffsetView>(x1, 1)->getId();
  VarId viewPlus2 = engine->makeIntView<IntOffsetView>(x2, 2)->getId();
  VarId viewPlus3 = engine->makeIntView<IntOffsetView>(x3, 3)->getId();

  engine->makeInvariant<ElementVar>(
      i1, std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}), x1);
  engine->makeInvariant<ElementVar>(
      i2, std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}), x2);
  engine->makeInvariant<ElementVar>(
      i3, std::vector<VarId>({base, viewPlus1, viewPlus2, viewPlus3}), x3);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1, 1}),
                                std::vector<VarId>({x1, x2, x3}), output);

  engine->close();

  EXPECT_EQ(engine->getCommittedValue(output), 7);

  engine->beginMove();
  engine->setValue(i1, 3);
  engine->setValue(i2, 0);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  EXPECT_EQ(engine->getNewValue(base), 1);
  EXPECT_EQ(engine->getNewValue(i1), 3);
  EXPECT_EQ(engine->getNewValue(i2), 0);
  EXPECT_EQ(engine->getNewValue(i3), 2);
  EXPECT_EQ(engine->getNewValue(x1), 6);  // x1 = x3plus3 = x3 + 3
  EXPECT_EQ(engine->getNewValue(x2), 1);  // x2 = base = 1
  EXPECT_EQ(engine->getNewValue(x3), 3);  // x3 = x2plus2 = x2 + 2

  EXPECT_EQ(engine->getNewValue(output), 10);

  engine->beginMove();
  engine->setValue(base, 3);
  engine->endMove();

  engine->beginQuery();
  engine->query(output);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(base), 3);
  EXPECT_EQ(engine->getNewValue(i1), 3);
  EXPECT_EQ(engine->getNewValue(i2), 0);
  EXPECT_EQ(engine->getNewValue(i3), 2);
  EXPECT_EQ(engine->getNewValue(x1), 8);  // x1 = x3plus3 = x3 + 3
  EXPECT_EQ(engine->getNewValue(x2), 3);  // x2 = base = 1
  EXPECT_EQ(engine->getNewValue(x3), 5);  // x3 = x2plus2 = x2 + 2
  EXPECT_EQ(engine->getNewValue(output), 16);

  engine->beginMove();
  engine->setValue(i2, 1);
  engine->setValue(i3, 0);
  engine->setValue(base, 2);
  engine->endMove();

  engine->beginQuery();
  engine->query(output);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(output), 13);

  engine->beginMove();
  engine->setValue(i2, 1);
  engine->setValue(i3, 0);
  engine->setValue(base, 2);
  engine->endMove();

  engine->beginCommit();
  engine->query(output);
  engine->endCommit();

  EXPECT_EQ(engine->getNewValue(output), 13);
}

}  // namespace
