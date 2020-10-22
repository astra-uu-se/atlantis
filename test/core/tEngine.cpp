#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class MockInvariantSimple : public Invariant {
 public:
  bool m_initialized = false;

  MockInvariantSimple() : Invariant(NULL_ID) {}

  void init(Timestamp, Engine&) override { m_initialized = true; }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));

  MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& e),
              (override));

  MOCK_METHOD(void, notifyIntChanged,
              (Timestamp t, Engine& e, LocalId id, Int newValue), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
};

class MockIntVarView : public IntVarView {
  public:
   bool m_initialized = false;
   int recomputeCount;
   int commitValueCount;
   MockIntVarView(const VarId parentId) : IntVarView(parentId), recomputeCount(0), commitValueCount(0) {}
   void init(Timestamp t, Engine&, Int sourceVal, Int sourceCommittedVal) override {
     m_savedInt.setValue(t, sourceVal);
     m_savedInt.commitValue(sourceCommittedVal);
     m_initialized = true;
   }
   void recompute(Timestamp t, Int parentVal) {
    m_savedInt.setValue(t, parentVal);
    ++recomputeCount;
   }
   void recompute(Timestamp t, Int parentVal, Int parentCommittedVal) override {
    m_savedInt.setValue(t, parentVal);
    m_savedInt.commitValue(parentCommittedVal);
    ++recomputeCount;
   }
   void commitValue(Int parentVal) override {
    m_savedInt.commitValue(parentVal);
    ++commitValueCount;
   }
};

class MockInvariantAdvanced : public Invariant {
 public:
  bool m_initialized = false;
  std::vector<VarId> m_inputs;
  VarId m_output;

  MockInvariantAdvanced(std::vector<VarId>&& t_inputs, VarId t_output)
      : Invariant(NULL_ID), m_inputs(std::move(t_inputs)), m_output(t_output) {}

  void init(Timestamp, Engine& e) override {
    assert(m_id != NULL_ID);

    e.registerDefinedVariable(m_output, m_id);
    for (size_t i = 0; i < m_inputs.size(); ++i) {
      e.registerInvariantDependsOnVar(m_id, m_inputs[i], LocalId(i));
    }
    m_initialized = true;
  }

  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));
  MOCK_METHOD(VarId, getNextDependency, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentDependencyChanged, (Timestamp, Engine& e),
              (override));
  MOCK_METHOD(void, notifyIntChanged,
              (Timestamp t, Engine& e, LocalId id, Int newValue), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
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

  ASSERT_TRUE(invariant->m_initialized);

  engine->close();
  EXPECT_EQ(engine->getStore().getNumVariables(), intVarCount);
  EXPECT_EQ(engine->getStore().getNumInvariants(), size_t(1));
}

TEST_F(EngineTest, CreateIntVarViews) {
  engine->open();

  VarId var = engine->makeIntVar(5, 0, 10);

  std::vector<std::shared_ptr<MockIntVarView>> varViews;
  for (size_t i = 0; i < 10; ++i) {
    varViews.push_back(engine->makeIntVarView<MockIntVarView>(
        i == 0
          ? var
          : varViews[i - 1]->getId()
    ));
    ASSERT_TRUE(varViews[i]->m_initialized);
  }

  engine->close();

  for (auto varView : varViews) {
    EXPECT_EQ(varView->recomputeCount, 0);
  }

  Int val(10);
  engine->beginMove();
  engine->setValue(var, val);
  engine->endMove();
  
  Timestamp t = engine->getTmpTimestamp(var);
  
  for (auto varView : varViews) {
    EXPECT_EQ(varView->recomputeCount, 0);
    VarId id = varView->getId();
    EXPECT_EQ(engine->getValue(t, id), val);
    EXPECT_EQ(varView->recomputeCount, 1);
  }

  for (auto varView : varViews) {
    EXPECT_EQ(varView->recomputeCount, 1);
  }

  val = Int(0);
  engine->beginMove();
  engine->setValue(var, val);
  engine->endMove();
  t = engine->getTmpTimestamp(var);
  
  EXPECT_EQ(varViews[varViews.size() - 1]->recomputeCount, 1);
  VarId lastId = varViews[varViews.size() - 1]->getId();
  EXPECT_EQ(engine->getValue(t, lastId), val);
  EXPECT_EQ(varViews[varViews.size() - 1]->recomputeCount, 2);

  for (int i = varViews.size() - 1; i >= 0; --i) {
    EXPECT_EQ(varViews[i]->recomputeCount, 2);
    VarId id = varViews[i]->getId();
    EXPECT_EQ(engine->getValue(t, id), val);
    EXPECT_EQ(varViews[i]->recomputeCount, 2);
  }

  for (auto varView : varViews) {
    EXPECT_EQ(varView->recomputeCount, 2);
  }

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

  ASSERT_TRUE(invariant->m_initialized);

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
  Timestamp moveTimestamp = engine->getCurrentTime();

  engine->setValue(a, -1);
  engine->setValue(b, -2);
  engine->setValue(c, -3);
  engine->endMove();

  if (engine->mode == PropagationEngine::PropagationMode::TOP_DOWN) {
    EXPECT_CALL(*invariant, getNextDependency(moveTimestamp, testing::_))
        .Times(0);

    EXPECT_CALL(*invariant,
                notifyCurrentDependencyChanged(moveTimestamp, testing::_))
        .Times(0);
  } else if (engine->mode == PropagationEngine::PropagationMode::BOTTOM_UP) {
    EXPECT_CALL(*invariant, getNextDependency(moveTimestamp, testing::_))
        .WillOnce(Return(a))
        .WillOnce(Return(b))
        .WillOnce(Return(c))
        .WillRepeatedly(Return(NULL_ID));

    EXPECT_CALL(*invariant,
                notifyCurrentDependencyChanged(moveTimestamp, testing::_))
        .Times(3);
  } else if (engine->mode == PropagationEngine::PropagationMode::MIXED) {
    EXPECT_EQ(0, 1);  // TODO: define the test case for mixed mode.
  }

  engine->beginQuery();
  engine->query(output);
  engine->endQuery();
  std::cout << "foo";
}

}  // namespace
