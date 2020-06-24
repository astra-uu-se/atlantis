#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/linear.hpp"
#include "propagation/bottomUpPropagationGraph.hpp"

namespace {

class BottomUpPropagationTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  BottomUpPropagationGraph* pg;
  std::unique_ptr<Engine> e;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<Engine>();
    pg = &(e->getPropGraph());
  }
};

TEST_F(BottomUpPropagationTest, RegisterVariableAndInvariant) {
  e->open();

  auto v1 = e->makeIntVar(0, -100, 100);
  auto v2 = e->makeIntVar(0, -100, 100);
  auto v3 = e->makeIntVar(0, -100, 100);
  auto v4 = e->makeIntVar(0, -100, 100);
  auto v5 = e->makeIntVar(0, -100, 100);
  auto v6 = e->makeIntVar(0, -100, 100);

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1},
                           std::vector<VarId>{v1, v2, v3}, v4);

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1, 1},
                           std::vector<VarId>{v1, v2, v3, v4}, v5);

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1, 1, 1},
                           std::vector<VarId>{v1, v2, v3, v4, v5}, v6);

  e->close();

  EXPECT_EQ(pg->getNumInvariants(), 3);
  EXPECT_EQ(pg->getNumVariables(), 6);
}

TEST_F(BottomUpPropagationTest, SimplePropagate1) {
  e->open();

  auto v1 = e->makeIntVar(0, -100, 100);
  auto v2 = e->makeIntVar(0, -100, 100);
  auto v3 = e->makeIntVar(0, -100, 100);
  auto v4 = e->makeIntVar(0, -100, 100);
  auto v5 = e->makeIntVar(0, -100, 100);
  auto v6 = e->makeIntVar(0, -100, 100);

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1},
                           std::vector<VarId>{v1, v2, v3}, v4);

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1, 1},
                           std::vector<VarId>{v1, v2, v3, v4}, v5);

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1, 1, 1},
                           std::vector<VarId>{v1, v2, v3, v4, v5}, v6);

  e->close();

  EXPECT_EQ(e->getValue(v4), 0);
  EXPECT_EQ(e->getValue(v5), 0);
  EXPECT_EQ(e->getValue(v6), 0);

  e->beginMove();
  e->setValue(v1, 1);
  e->setValue(v2, 1);
  e->setValue(v3, 1);
  e->endMove();

  e->beginQuery();
  e->query(v4);
  e->query(v5);
  e->query(v6);
  e->endQuery();

  EXPECT_EQ(e->getValue(v4), 3);
  EXPECT_EQ(e->getValue(v5), 6);
  EXPECT_EQ(e->getValue(v6), 12);

  std::uniform_int_distribution<> distribution(-10000, 10000);
  for (size_t i = 0; i < 1000; ++i) {
    Int a = distribution(gen);
    Int b = distribution(gen);
    Int c = distribution(gen);

    e->beginMove();
    e->setValue(v1, a);
    e->setValue(v2, b);
    e->setValue(v3, c);
    e->endMove();

    e->beginQuery();
    e->query(v4);
    e->query(v5);
    e->query(v6);
    e->endQuery();

    ASSERT_EQ(e->getValue(v4), a + b + c);
    ASSERT_EQ(e->getValue(v5), 2 * (a + b + c));
    ASSERT_EQ(e->getValue(v6), 4 * (a + b + c));
  }
}

TEST_F(BottomUpPropagationTest, SimplePropagate2) {
  e->open();

  auto v1 = e->makeIntVar(0, -100, 100);
  auto v2 = e->makeIntVar(0, -100, 100);
  auto v3 = e->makeIntVar(0, -100, 100);
  auto v4 = e->makeIntVar(0, -100, 100);
  auto v5 = e->makeIntVar(0, -100, 100);
  auto v6 = e->makeIntVar(0, -100, 100);

  auto v7 = e->makeIntVar(0, -200, 200);
  e->makeInvariant<Linear>(std::vector<Int>{1, 2}, std::vector<VarId>{v1, v2},
                           v7);

  auto v8 = e->makeIntVar(0, -200, 200);
  e->makeInvariant<Linear>(std::vector<Int>{3, 4}, std::vector<VarId>{v3, v4},
                           v8);

  auto v9 = e->makeIntVar(0, -200, 200);
  e->makeInvariant<Linear>(std::vector<Int>{5, 6}, std::vector<VarId>{v5, v6},
                           v9);

  auto v10 = e->makeIntVar(0, -600, 600);
  e->makeInvariant<Linear>(std::vector<Int>{1, -1, 2},
                           std::vector<VarId>{v7, v8, v9}, v10);

  auto v11 = e->makeIntVar(0, -800, 800);
  e->makeInvariant<Linear>(std::vector<Int>{-3, 4}, std::vector<VarId>{v7, v10},
                           v11);

  auto v12 = e->makeIntVar(0, -400, 400);
  e->makeInvariant<Linear>(std::vector<Int>{1, 1}, std::vector<VarId>{v8, v9},
                           v12);

  auto v13 = e->makeIntVar(0, -1200, 1200);
  e->makeInvariant<Linear>(std::vector<Int>{10, -10}, std::vector<VarId>{v11, v12},
                           v13);

  e->close();

  ASSERT_EQ(e->getValue(v13), 0);


  std::uniform_int_distribution<> distribution(-10000, 10000);
  for (size_t i = 0; i < 1000; ++i) {
    Int a1 = distribution(gen);
    Int a2 = distribution(gen);
    Int a3 = distribution(gen);
    Int a4 = distribution(gen);
    Int a5 = distribution(gen);
    Int a6 = distribution(gen);

    Int x7 = 1*a1+2*a2;
    Int x8 = 3*a3+4*a4;
    Int x9 = 5*a5+6*a6;

    Int x10 = 1*x7-x8+2*x9;

    Int x11 = -3*x7+4*x10;

    Int x12 = x8+x9;


    Int x13 = 10*x11-10*x12;


    e->beginMove();
    e->setValue(v1, a1);
    e->setValue(v2, a2);
    e->setValue(v3, a3);
    e->setValue(v4, a4);
    e->setValue(v5, a5);
    e->setValue(v6, a6);
    e->endMove();

    e->beginQuery();
    e->query(v13);
    e->endQuery();

    ASSERT_EQ(e->getValue(v7), x7);
    ASSERT_EQ(e->getValue(v8), x8);
    ASSERT_EQ(e->getValue(v9), x9);
    ASSERT_EQ(e->getValue(v10), x10);
    ASSERT_EQ(e->getValue(v11), x11);
    ASSERT_EQ(e->getValue(v12), x12);
    ASSERT_EQ(e->getValue(v13), x13);
  }
}

}  // namespace