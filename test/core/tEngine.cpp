#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/linear.hpp"
#include "propagation/topDownPropagationGraph.hpp"

namespace {

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

  auto a = engine->makeIntVar(1);
  auto b = engine->makeIntVar(1);
  auto c = engine->makeIntVar(1);
  auto d = engine->makeIntVar(1);
  auto e = engine->makeIntVar(1);



  // TODO: use some other invariants...
  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({a, b}), c);
  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({b, c}), d);
  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, d}), e);

  engine->close();
  EXPECT_EQ(engine->getStore().getNumVariables(), 5);
  EXPECT_EQ(engine->getStore().getNumInvariants(), 3);
}

TEST_F(EngineTest, SimplePropagation) {

  engine->open();
  
  // 5a+2b+3c -> d
  // b-c -> e
  // a+2e -> f

  auto a = engine->makeIntVar(1);
  auto b = engine->makeIntVar(1);
  auto c = engine->makeIntVar(1);
  auto d = engine->makeIntVar(1);
  auto e = engine->makeIntVar(1);
  auto f = engine->makeIntVar(1);

  engine->makeInvariant<Linear>(std::vector<Int>({5, 2, 3}),
                                std::vector<VarId>({a, b, c}), d);
  engine->makeInvariant<Linear>(std::vector<Int>({1, -1}),
                                std::vector<VarId>({b, c}), e);
  engine->makeInvariant<Linear>(std::vector<Int>({1, 2}),
                                std::vector<VarId>({a, e}), f);


  engine->close();

  // All variables are initialised to 1:
  // d = 5+2+3 = 10
  // e = 1-1 = 0
  // f = 1+0 = 1

  ASSERT_EQ(engine->getValue(d), 10);
  ASSERT_EQ(engine->getValue(e), 0);
  ASSERT_EQ(engine->getValue(f), 1);

  //Change c to -4
  // 5+2-12 -> d = -5
  // 1+4 -> e = 5
  // 1+10 -> f = 11
  engine->beginMove();
  engine->setValue(c, -4);
  engine->endMove();
  engine->propagate();

  ASSERT_EQ(engine->getValue(d), -5);
  ASSERT_EQ(engine->getValue(e), 5);
  ASSERT_EQ(engine->getValue(f), 11);

}

}  // namespace