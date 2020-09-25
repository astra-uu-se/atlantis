#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/elementVar.hpp"
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

  EXPECT_EQ(pg->getNumInvariants(), static_cast<size_t>(3));
  EXPECT_EQ(pg->getNumVariables(), static_cast<size_t>(6));
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

  EXPECT_EQ(e->getValue(v4), static_cast<Int>(0));
  EXPECT_EQ(e->getValue(v5), static_cast<Int>(0));
  EXPECT_EQ(e->getValue(v6), static_cast<Int>(0));

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

  EXPECT_EQ(e->getValue(v4), static_cast<Int>(3));
  EXPECT_EQ(e->getValue(v5), static_cast<Int>(6));
  EXPECT_EQ(e->getValue(v6), static_cast<Int>(12));

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
    ASSERT_EQ(e->getValue(v5), static_cast<Int>(2) * (a + b + c));
    ASSERT_EQ(e->getValue(v6), static_cast<Int>(4) * (a + b + c));
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
  e->makeInvariant<Linear>(std::vector<Int>{10, -10},
                           std::vector<VarId>{v11, v12}, v13);

  e->close();

  ASSERT_EQ(e->getValue(v13), static_cast<Int>(0));

  std::uniform_int_distribution<> distribution(-10000, 10000);
  for (size_t i = 0; i < 1000; ++i) {
    Int a1 = distribution(gen);
    Int a2 = distribution(gen);
    Int a3 = distribution(gen);
    Int a4 = distribution(gen);
    Int a5 = distribution(gen);
    Int a6 = distribution(gen);

    Int x7 = 1 * a1 + 2 * a2;
    Int x8 = 3 * a3 + 4 * a4;
    Int x9 = 5 * a5 + 6 * a6;

    Int x10 = 1 * x7 - x8 + 2 * x9;

    Int x11 = -3 * x7 + 4 * x10;

    Int x12 = x8 + x9;

    Int x13 = 10 * x11 - 10 * x12;

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

TEST_F(BottomUpPropagationTest, QueryInput) {
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

  e->beginMove();
  e->setValue(v1, 1);
  e->setValue(v2, 2);
  e->endMove();

  e->beginQuery();
  e->query(v1);
  e->query(v2);
  e->query(v3);
  e->endQuery();

  EXPECT_EQ(e->getValue(v1), static_cast<Int>(1));
  EXPECT_EQ(e->getValue(v2), static_cast<Int>(2));
  EXPECT_EQ(e->getValue(v3), static_cast<Int>(0));
}

TEST_F(BottomUpPropagationTest, SimpleCommit) {
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

  // Make a bunch of probes
  std::uniform_int_distribution<> distribution(-10000, 10000);
  for (size_t i = 0; i < 100; ++i) {
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
  }

  e->beginMove();
  e->setValue(v1, 1);
  e->setValue(v2, 2);
  e->endMove();

  // todo: current implementation commits all variables but in general we can
  // only expect that input variables and queried variables get committed (as
  // committing should be lazy)

  e->beginCommit();
  e->query(v6);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(v1), static_cast<Int>(1));
  EXPECT_EQ(e->getCommittedValue(v2), static_cast<Int>(2));
  EXPECT_EQ(e->getCommittedValue(v3), static_cast<Int>(0));

  EXPECT_EQ(e->getCommittedValue(v6), static_cast<Int>(4 * (1 + 2 + 0)));

  e->beginMove();
  e->setValue(v1, 0);
  e->setValue(v2, 0);
  e->endMove();
  e->beginCommit();
  e->query(v6);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(v1), static_cast<Int>(0));
  EXPECT_EQ(e->getCommittedValue(v2), static_cast<Int>(0));
  EXPECT_EQ(e->getCommittedValue(v3), static_cast<Int>(0));

  EXPECT_EQ(e->getCommittedValue(v6), static_cast<Int>(0));
}

TEST_F(BottomUpPropagationTest, DynamicCommit) {
  e->open();

  auto v1 = e->makeIntVar(1, -100, 100);
  auto v2 = e->makeIntVar(2, -100, 100);
  auto v3 = e->makeIntVar(3, -100, 100);
  auto v4 = e->makeIntVar(4, -100, 100);

  auto w1 = e->makeIntVar(0, -100, 100);
  auto w2 = e->makeIntVar(0, -100, 100);
  auto w3 = e->makeIntVar(0, -100, 100);
  auto w4 = e->makeIntVar(0, -100, 100);

  auto output = e->makeIntVar(0, -100, 100);
  auto idx = e->makeIntVar(0, 0, 3);

  e->makeInvariant<ElementVar>(idx, std::vector<VarId>{w1, w2, w3, w4}, output);

  e->makeInvariant<Linear>(std::vector<Int>{2}, std::vector<VarId>{v1}, w1);
  e->makeInvariant<Linear>(std::vector<Int>{2}, std::vector<VarId>{v2}, w2);
  e->makeInvariant<Linear>(std::vector<Int>{2}, std::vector<VarId>{v3}, w3);
  e->makeInvariant<Linear>(std::vector<Int>{2}, std::vector<VarId>{v4}, w4);

  e->close();

  ASSERT_EQ(e->getCommittedValue(output), static_cast<Int>(2 * 1));

  // Make a bunch of probes
  auto probe = [&]() {
    std::uniform_int_distribution<> distribution(-10000, 10000);
    std::uniform_int_distribution<> idx_dist(0, 3);
    for (size_t i = 0; i < 100; ++i) {
      Int a = distribution(gen);
      Int b = distribution(gen);
      Int c = distribution(gen);
      Int d = distribution(gen);
      Int j = idx_dist(gen);

      e->beginMove();
      e->setValue(v1, a);
      e->setValue(v2, b);
      e->setValue(v3, c);
      e->setValue(v4, d);
      e->setValue(idx, j);
      e->endMove();

      e->beginQuery();
      e->query(output);
      e->endQuery();
      ASSERT_EQ(e->getValue(output), (std::vector<Int>{a, b, c, d}).at(j) * 2);
    }
  };

  probe();

  e->beginMove();
  e->setValue(v3, 10);
  e->endMove();

  e->beginCommit();
  e->query(output);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(v3), static_cast<Int>(10));
  EXPECT_EQ(e->getCommittedValue(output), static_cast<Int>(2 * (1)));

  probe();

  e->beginMove();
  e->setValue(idx, 2);
  e->endMove();

  e->beginCommit();
  e->query(output);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(v3), static_cast<Int>(10));
  EXPECT_EQ(e->getCommittedValue(idx), static_cast<Int>(2));
  EXPECT_EQ(e->getCommittedValue(output), static_cast<Int>(2 * (10)));
}

TEST_F(BottomUpPropagationTest, DynamicCommit2) {
  e->open();

  auto output = e->makeIntVar(0, -1000, 1000);  // 1
  auto idx1 = e->makeIntVar(0, 0, 2);           // 2

  auto v11 = e->makeIntVar(11, -100, 1000);  // 3
  auto v12 = e->makeIntVar(12, -100, 1000);  // 4
  auto v13 = e->makeIntVar(13, -100, 1000);  // 5

  e->makeInvariant<ElementVar>(idx1, std::vector<VarId>{v11, v12, v13}, output);

  auto idx2 = e->makeIntVar(0, 0, 2);        // 6
  auto v21 = e->makeIntVar(21, -100, 1000);  // 7
  auto v22 = e->makeIntVar(22, -100, 1000);  // 8
  auto v23 = e->makeIntVar(23, -100, 1000);  // 9

  e->makeInvariant<ElementVar>(idx2, std::vector<VarId>{v21, v22, v23}, v11);

  auto idx3 = e->makeIntVar(0, 0, 2);        // 10
  auto v31 = e->makeIntVar(31, -100, 1000);  // 11
  auto v32 = e->makeIntVar(32, -100, 1000);
  auto v33 = e->makeIntVar(33, -100, 1000);

  e->makeInvariant<ElementVar>(idx3, std::vector<VarId>{v31, v32, v33}, v12);

  auto idx4 = e->makeIntVar(0, 0, 2);
  auto v41 = e->makeIntVar(41, -100, 1000);
  auto v42 = e->makeIntVar(42, -100, 1000);
  auto v43 = e->makeIntVar(43, -100, 1000);

  e->makeInvariant<ElementVar>(idx4, std::vector<VarId>{v41, v42, v43}, v13);

  e->close();

  ASSERT_EQ(e->getCommittedValue(output), static_cast<Int>(21));

  // Make a bunch of probes
  auto probe = [&]() {
    std::uniform_int_distribution<> distribution(0, 50);
    std::uniform_int_distribution<> idx_dist(0, 2);
    for (size_t i = 0; i < 100; ++i) {
      Int a2 = distribution(gen);
      Int b2 = distribution(gen);
      Int c2 = distribution(gen);
      Int a3 = distribution(gen);
      Int b3 = distribution(gen);
      Int c3 = distribution(gen);
      Int a4 = distribution(gen);
      Int b4 = distribution(gen);
      Int c4 = distribution(gen);
      Int j1 = idx_dist(gen);
      Int j2 = idx_dist(gen);
      Int j3 = idx_dist(gen);
      Int j4 = idx_dist(gen);

      e->beginMove();
      e->setValue(idx1, j1);

      e->setValue(v21, a2);
      e->setValue(v22, b2);
      e->setValue(v23, c2);
      e->setValue(idx2, j2);

      e->setValue(v31, a3);
      e->setValue(v32, b3);
      e->setValue(v33, c3);
      e->setValue(idx3, j3);

      e->setValue(v41, a4);
      e->setValue(v42, b4);
      e->setValue(v43, c4);
      e->setValue(idx4, j4);
      e->endMove();

      e->beginQuery();
      e->query(output);
      e->endQuery();

      std::vector<std::vector<Int>> data{std::vector<Int>{a2, b2, c2},
                                         std::vector<Int>{a3, b3, c3},
                                         std::vector<Int>{a4, b4, c4}};
      std::vector<Int> indices{j2, j3, j4};
      ASSERT_EQ(e->getValue(output), data.at(j1).at(indices.at(j1)));
    }
  };

  probe();

  e->beginMove();
  e->setValue(v32, 132);
  e->setValue(idx3, 1);
  e->endMove();

  e->beginCommit();
  e->query(output);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(v32), static_cast<Int>(132));
  EXPECT_EQ(e->getCommittedValue(idx3), static_cast<Int>(1));
  EXPECT_EQ(e->getCommittedValue(output), static_cast<Int>(21));

  // probe();

  e->beginMove();
  e->setValue(idx1, 1);
  e->setValue(v43, 143);
  e->endMove();

  e->beginCommit();
  e->query(output);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(v32), static_cast<Int>(132));
  EXPECT_EQ(e->getCommittedValue(output), static_cast<Int>(132));

  probe();

  e->beginMove();
  e->setValue(idx1, 2);
  e->setValue(idx4, 2);
  e->endMove();

  e->beginCommit();
  e->query(output);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(output), static_cast<Int>(143));
}

TEST_F(BottomUpPropagationTest, TallTree) {
  e->open();

  auto vA1 = e->makeIntVar(1, -100, 1000);  // 1
  auto vA2 = e->makeIntVar(2, -100, 1000);  // 2
  auto vA3 = e->makeIntVar(3, -100, 1000);  // 3
  auto sumA = e->makeIntVar(0, -1000, 1000);  // 4

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1},
                           std::vector<VarId>{vA1, vA2, vA3}, sumA);

  auto vB1 = e->makeIntVar(1, -100, 1000);  // 5
  auto vB2 = e->makeIntVar(2, -100, 1000);  // 6
  auto vB3 = e->makeIntVar(3, -100, 1000);  // 7
  auto sumB = e->makeIntVar(0, -1000, 1000);  // 8

  e->makeInvariant<Linear>(std::vector<Int>{1, 1, 1},
                           std::vector<VarId>{vB1, vB2, vB3}, sumB);

  auto output = e->makeIntVar(0, -1000, 1000);  // 9

  e->makeInvariant<Linear>(std::vector<Int>{1,1},
                           std::vector<VarId>{sumA, sumB}, output);

  e->close();

  ASSERT_EQ(e->getCommittedValue(output), static_cast<Int>(12));

  // Make a bunch of probes
  
  e->beginMove();
  e->setValue(vA1, 10);
  e->setValue(vB1, 10);
  e->endMove();

  e->beginCommit();
  e->query(output);
  e->endCommit();

  EXPECT_EQ(e->getCommittedValue(sumA), static_cast<Int>(15));
  EXPECT_EQ(e->getCommittedValue(sumB), static_cast<Int>(15));
  EXPECT_EQ(e->getCommittedValue(output), static_cast<Int>(30));
}

}  // namespace