#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/linear.hpp"
#include "views/intMaxView.hpp"

namespace {

class IntMaxViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }
};

RC_GTEST_FIXTURE_PROP(IntMaxViewTest, shouldAlwaysBeMax, (Int a, Int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId viewId = engine->makeIntView<IntMaxView>(varId, b);
  RC_ASSERT(engine->committedValue(viewId) == std::max(a, b));
}

TEST_F(IntMaxViewTest, CreateIntMaxView) {
  engine->open();

  const VarId var = engine->makeIntVar(10, 0, 10);
  const VarId viewOfVar = engine->makeIntView<IntMaxView>(var, 25);
  const VarId viewOfView = engine->makeIntView<IntMaxView>(viewOfVar, 50);

  EXPECT_EQ(engine->committedValue(viewOfVar), Int(25));
  EXPECT_EQ(engine->committedValue(viewOfView), Int(50));

  engine->close();
}

TEST_F(IntMaxViewTest, ComputeBounds) {
  engine->open();
  auto a = engine->makeIntVar(20, -100, 100);
  auto b = engine->makeIntVar(20, -100, 100);

  const VarId va = engine->makeIntView<IntMaxView>(a, 10);
  const VarId vb = engine->makeIntView<IntMaxView>(b, 200);

  EXPECT_EQ(engine->lowerBound(va), Int(-100));
  EXPECT_EQ(engine->lowerBound(vb), Int(-100));
  EXPECT_EQ(engine->upperBound(va), Int(10));
  EXPECT_EQ(engine->upperBound(vb), Int(100));

  engine->close();

  EXPECT_EQ(engine->lowerBound(va), Int(-100));
  EXPECT_EQ(engine->lowerBound(vb), Int(-100));
  EXPECT_EQ(engine->upperBound(va), Int(10));
  EXPECT_EQ(engine->upperBound(vb), Int(100));
}

TEST_F(IntMaxViewTest, RecomputeIntMaxView) {
  engine->open();
  const VarId a = engine->makeIntVar(20, -100, 100);
  const VarId b = engine->makeIntVar(20, -100, 100);
  const VarId sum = engine->makeIntVar(0, -100, 100);

  auto linear = engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                              std::vector<VarId>({a, b}), sum);

  const VarId viewOfVar = engine->makeIntView<IntMaxView>(sum, 10);
  const VarId viewOfView = engine->makeIntView<IntMaxView>(viewOfVar, 15);

  EXPECT_EQ(engine->currentValue(viewOfVar), Int(10));
  EXPECT_EQ(engine->currentValue(viewOfView), Int(15));

  engine->close();

  EXPECT_EQ(engine->currentValue(sum), Int(40));
  EXPECT_EQ(engine->currentValue(viewOfVar), Int(40));
  EXPECT_EQ(engine->currentValue(viewOfView), Int(40));

  engine->beginMove();
  engine->setValue(a, 1);
  engine->setValue(b, 1);
  engine->endMove();

  engine->beginProbe();
  engine->query(sum);
  engine->endProbe();

  EXPECT_EQ(engine->currentValue(sum), Int(2));
  EXPECT_EQ(engine->currentValue(viewOfVar), Int(10));
  EXPECT_EQ(engine->currentValue(viewOfView), Int(15));
}

TEST_F(IntMaxViewTest, PropagateIntViews) {
  engine->open();
  auto a = engine->makeIntVar(20, -100, 100);
  auto b = engine->makeIntVar(20, -100, 100);
  auto sum1 = engine->makeIntVar(0, -100, 100);
  // a + b = sum1
  auto c = engine->makeIntVar(20, -100, 100);
  auto d = engine->makeIntVar(20, -100, 100);
  auto sum2 = engine->makeIntVar(0, -100, 100);
  // c + d = sum2
  auto sum3 = engine->makeIntVar(0, -100, 100);
  // sum1 + sum2 = sum2

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({a, b}), sum1);

  engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, d}), sum2);

  const VarId sum1View = engine->makeIntView<IntMaxView>(sum1, 45);
  const VarId sum2View = engine->makeIntView<IntMaxView>(sum2, 20);

  auto linear3 = engine->makeInvariant<Linear>(
      std::vector<Int>({1, 1}), std::vector<VarId>({sum1View, sum2View}), sum3);

  std::vector<VarId> sum3views;
  VarId prev = sum3;
  for (size_t i = 0; i < 10; ++i) {
    sum3views.emplace_back(engine->makeIntView<IntMaxView>(prev, 80 + i));
    prev = sum3views[i];
  }

  EXPECT_EQ(engine->committedValue(sum1), Int(0));
  EXPECT_EQ(engine->committedValue(sum2), Int(0));
  EXPECT_EQ(engine->committedValue(sum1View), Int(45));
  EXPECT_EQ(engine->committedValue(sum2View), Int(20));

  engine->close();

  // a + b = 20 + 20 = sum1 = 40
  EXPECT_EQ(engine->currentValue(sum1), Int(40));
  // sum1 = 40 -> sum1View = min(20, 40) = 20
  EXPECT_EQ(engine->currentValue(sum1View), Int(45));
  // c + d = 20 + 20 = sum2 = 40
  EXPECT_EQ(engine->currentValue(sum2), Int(40));
  // sum2 = 40 -> sum2View = min(20, 40) = 20
  EXPECT_EQ(engine->currentValue(sum2View), Int(40));
  // sum3 = sum1view + sum2view = 45 + 40 = 85
  EXPECT_EQ(engine->currentValue(sum3), Int(85));

  for (size_t i = 0; i < 10; ++i) {
    if (i == 0) {
      EXPECT_EQ(engine->committedValue(sum3views[i]), Int(85));
    } else {
      EXPECT_EQ(engine->committedValue(sum3views[i]),
                std::max({Int(80 + i), Int(80 + i - 1), Int(85)}));
    }
  }

  engine->beginMove();
  engine->setValue(a, 5);
  engine->setValue(b, 5);
  engine->setValue(c, 5);
  engine->setValue(d, 5);
  engine->endMove();

  EXPECT_EQ(engine->currentValue(a), Int(5));
  EXPECT_EQ(engine->currentValue(b), Int(5));
  EXPECT_EQ(engine->currentValue(c), Int(5));
  EXPECT_EQ(engine->currentValue(d), Int(5));

  engine->beginCommit();
  engine->query(sum1);
  engine->query(sum2);
  engine->query(sum3);
  engine->endCommit();

  // a + b = 5 + 5 = sum1 = 10
  EXPECT_EQ(engine->committedValue(sum1), Int(10));
  // c + d = 5 + 5 = sum2 = 10
  EXPECT_EQ(engine->committedValue(sum2), Int(10));

  // sum1 = 10 -> sum1View = min(20, 10) = 10
  EXPECT_EQ(engine->committedValue(sum1View), Int(45));
  // sum2 = 10 -> sum2View = min(20, 10) = 10
  EXPECT_EQ(engine->committedValue(sum2View), Int(20));

  // sum3 = sum1view + sum2view = 45 + 20 = 65

  EXPECT_EQ(engine->committedValue(sum3), Int(65));

  for (size_t i = 0; i < sum3views.size(); ++i) {
    EXPECT_EQ(engine->committedValue(sum3views[i]),
              std::max({Int(80 + i), Int(80 + i - 1), Int(65)}));
  }
}

}  // namespace
