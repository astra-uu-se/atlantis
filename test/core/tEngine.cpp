#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "invariants/linear.hpp"
#include "propagation/topDownPropagationGraph.hpp"

using ::testing::Return;

namespace {

class MockInvariantSimple : public Invariant {
  public:
  bool m_initialized = false;

  MockInvariantSimple() : Invariant(NULL_ID) {}

  void init(const Timestamp&, Engine&) override {
    m_initialized = true;
  }

  MOCK_METHOD(void, recompute, (const Timestamp& timestamp, Engine& engine), (override));
  MOCK_METHOD(
    void, notifyIntChanged, (const Timestamp& t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data), (override)
  );
  MOCK_METHOD(
    void, commit, (const Timestamp& timestamp, Engine& engine), (override)
  );
};

class MockInvariantAdvanced : public Invariant {
  public:
  bool m_initialized = false;
  std::vector<VarId> m_inputs;
  VarId m_output;


  MockInvariantAdvanced(std::vector<VarId>&& t_inputs, VarId t_output)
    : Invariant(NULL_ID), m_inputs(std::move(t_inputs)), m_output(t_output) {}

  void init([[maybe_unused]] const Timestamp& t, Engine& e) override {
    assert(m_id != NULL_ID);

    e.registerDefinedVariable(m_output, m_id);
    for (size_t i = 0; i < m_inputs.size(); ++i) {
      e.registerInvariantDependsOnVar(m_id, m_inputs[i], LocalId(i), i);
    }
    m_initialized = true;
  }

  MOCK_METHOD(void, recompute, (const Timestamp& timestamp, Engine& engine), (override));
  MOCK_METHOD(
    void, notifyIntChanged, (const Timestamp& t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data), (override)
  );
  MOCK_METHOD(
    void, commit, (const Timestamp& timestamp, Engine& engine), (override)
  );
};

class EngineTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<Engine> engine;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<Engine>();
  }
};

TEST_F(EngineTest, CreateVariablesAndInvariant) {
  engine->open();

  int intVarCount = 10;
  for (int value = 0; value < intVarCount; ++value) {
    engine->makeIntVar(value);
  }

  // TODO: use some other invariants...
  auto invariant = engine->makeInvariant<MockInvariantSimple>();

  EXPECT_CALL(*invariant, recompute(0, testing::_))
    .Times(1);

  ASSERT_TRUE(invariant->m_initialized);

  engine->close();
  EXPECT_EQ(engine->getStore().getNumVariables(), intVarCount);
  EXPECT_EQ(engine->getStore().getNumInvariants(), 1);
}

TEST_F(EngineTest, RecomputeAndCommit) {
  engine->open();

  int intVarCount = 10;
  for (int value = 0; value < intVarCount; ++value) {
    engine->makeIntVar(value);
  }

  // TODO: use some other invariants...
  auto invariant = engine->makeInvariant<MockInvariantSimple>();

  EXPECT_CALL(*invariant, recompute(0, testing::_))
    .Times(1);

  ASSERT_TRUE(invariant->m_initialized);

  engine->close();
  EXPECT_EQ(engine->getStore().getNumVariables(), intVarCount);
  EXPECT_EQ(engine->getStore().getNumInvariants(), 1);
}

TEST_F(EngineTest, SimplePropagation) {
  engine->open();

  engine->open();
  
  VarId output = engine->makeIntVar(0);
  VarId a = engine->makeIntVar(1);
  VarId b = engine->makeIntVar(2);
  VarId c = engine->makeIntVar(3);
  
  auto invariant = engine->makeInvariant<MockInvariantAdvanced>(std::vector<VarId>({a, b, c}), output);
  
  EXPECT_CALL(*invariant, commit(0, testing::_))
    .Times(1);

  engine->close();

  EXPECT_CALL(*invariant, notifyIntChanged(1, testing::_, LocalId(0), 1, -1, 0))
    .Times(1);
  EXPECT_CALL(*invariant, notifyIntChanged(1, testing::_, LocalId(1), 2, -2, 1))
    .Times(1);
  EXPECT_CALL(*invariant, notifyIntChanged(1, testing::_, LocalId(2), 3, -3, 2))
    .Times(1);

  engine->beginMove();
  engine->setValue(a, -1);
  engine->setValue(b, -2);
  engine->setValue(c, -3);
  engine->endMove();

}

}  // namespace