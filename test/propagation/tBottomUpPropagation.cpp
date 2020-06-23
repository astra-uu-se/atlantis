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

  auto v1 = e->makeIntVar(0);
  auto v2 = e->makeIntVar(0);
  auto v3 = e->makeIntVar(0);
  auto v4 = e->makeIntVar(0);
  auto v5 = e->makeIntVar(0);
  auto v6 = e->makeIntVar(0);

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

  auto v1 = e->makeIntVar(0);
  auto v2 = e->makeIntVar(0);
  auto v3 = e->makeIntVar(0);
  auto v4 = e->makeIntVar(0);
  auto v5 = e->makeIntVar(0);
  auto v6 = e->makeIntVar(0);

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
}

}  // namespace