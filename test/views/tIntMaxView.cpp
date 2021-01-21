#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "rapidcheck/gtest.h"
#include "invariants/linear.hpp"
#include "views/intMaxView.hpp"

namespace {

class IntMaxViewTest : public ::testing::Test {
 protected:
  std::mt19937 gen;

  std::unique_ptr<PropagationEngine> engine;

  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    engine = std::make_unique<PropagationEngine>();
  }
};

RC_GTEST_FIXTURE_PROP(IntMaxViewTest,
                      shouldAlwaysBeMax,
                      (Int a, Int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  VarId varId = engine->makeIntVar(a, a, a);
  std::shared_ptr<IntMaxView> view = engine->makeIntView<IntMaxView>(varId, b);
  RC_ASSERT(view->getCommittedValue() == std::max(a, b));
}

TEST_F(IntMaxViewTest, CreateIntMaxView) {
  engine->open();

  const VarId var = engine->makeIntVar(10, 0, 10);
  std::shared_ptr<IntMaxView> viewOfVar =
      engine->makeIntView<IntMaxView>(var, 25);
  std::shared_ptr<IntMaxView> viewOfView =
      engine->makeIntView<IntMaxView>(viewOfVar->getId(), 50);

  EXPECT_EQ(viewOfVar->getCommittedValue(), Int(25));
  EXPECT_EQ(viewOfView->getCommittedValue(), Int(50));

  engine->close();
}

TEST_F(IntMaxViewTest, ComputeBounds) {
  engine->open();
  auto a = engine->makeIntVar(20, -100, 100);
  auto b = engine->makeIntVar(20, -100, 100);

  std::shared_ptr<IntMaxView> va = engine->makeIntView<IntMaxView>(a, 10);
  std::shared_ptr<IntMaxView> vb = engine->makeIntView<IntMaxView>(b, 200);

  EXPECT_EQ(engine->getLowerBound(va->getId()), Int(-100));
  EXPECT_EQ(engine->getLowerBound(vb->getId()), Int(-100));
  EXPECT_EQ(engine->getUpperBound(va->getId()), Int(10));
  EXPECT_EQ(engine->getUpperBound(vb->getId()), Int(100));

  engine->close();

  EXPECT_EQ(engine->getLowerBound(va->getId()), Int(-100));
  EXPECT_EQ(engine->getLowerBound(vb->getId()), Int(-100));
  EXPECT_EQ(engine->getUpperBound(va->getId()), Int(10));
  EXPECT_EQ(engine->getUpperBound(vb->getId()), Int(100));
}

TEST_F(IntMaxViewTest, RecomputeIntMaxView) {
  engine->open();
  auto a = engine->makeIntVar(20, -100, 100);
  auto b = engine->makeIntVar(20, -100, 100);
  auto sum = engine->makeIntVar(0, -100, 100);

  auto linear = engine->makeInvariant<Linear>(std::vector<Int>({1, 1}),
                                              std::vector<VarId>({a, b}), sum);

  std::shared_ptr<IntMaxView> viewOfVar =
      engine->makeIntView<IntMaxView>(sum, 10);
  std::shared_ptr<IntMaxView> viewOfView =
      engine->makeIntView<IntMaxView>(viewOfVar->getId(), 15);
  VarId viewOfVarId = viewOfVar->getId();
  VarId viewOfViewId = viewOfView->getId();

  EXPECT_EQ(engine->getNewValue(viewOfVarId), Int(10));
  EXPECT_EQ(engine->getNewValue(viewOfViewId), Int(15));

  engine->close();

  EXPECT_EQ(engine->getNewValue(sum), Int(40));
  EXPECT_EQ(engine->getNewValue(viewOfVarId), Int(40));
  EXPECT_EQ(engine->getNewValue(viewOfViewId), Int(40));

  engine->beginMove();
  engine->setValue(a, 1);
  engine->setValue(b, 1);
  engine->endMove();

  engine->beginQuery();
  engine->query(sum);
  engine->endQuery();

  EXPECT_EQ(engine->getNewValue(sum), Int(2));
  EXPECT_EQ(engine->getNewValue(viewOfVarId), Int(10));
  EXPECT_EQ(engine->getNewValue(viewOfViewId), Int(15));
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

  auto linear1 = engine->makeInvariant<Linear>(
      std::vector<Int>({1, 1}), std::vector<VarId>({a, b}), sum1);
  
  auto linear2 = engine->makeInvariant<Linear>(
      std::vector<Int>({1, 1}), std::vector<VarId>({c, d}), sum2);

  std::shared_ptr<IntMaxView> sum1View =
      engine->makeIntView<IntMaxView>(sum1, 45);
  std::shared_ptr<IntMaxView> sum2View =
      engine->makeIntView<IntMaxView>(sum2, 20);

  VarId sum1ViewId = sum1View->getId();
  VarId sum2ViewId = sum2View->getId();

  auto linear3 = engine->makeInvariant<Linear>(
      std::vector<Int>({1, 1}), std::vector<VarId>({sum1ViewId, sum2ViewId}),
      sum3);

  std::vector<VarId> sum3viewIds;
  std::vector<std::shared_ptr<IntMaxView>> sum3views;
  sum3viewIds.reserve(10);
  VarId prev = sum3;
  for (size_t i = 0; i < 10; ++i) {
    sum3views.emplace_back(engine->makeIntView<IntMaxView>(prev, 80 + i));
    sum3viewIds.emplace_back(sum3views[i]->getId());
    prev = sum3viewIds[i];
  }

  EXPECT_EQ(engine->getCommittedValue(sum1), Int(0));
  EXPECT_EQ(engine->getCommittedValue(sum2), Int(0));
  EXPECT_EQ(engine->getCommittedValue(sum1ViewId), Int(45));
  EXPECT_EQ(engine->getCommittedValue(sum2ViewId), Int(20));

  engine->close();

  // a + b = 20 + 20 = sum1 = 40
  EXPECT_EQ(engine->getNewValue(sum1), Int(40));
  // sum1 = 40 -> sum1View = min(20, 40) = 20
  EXPECT_EQ(engine->getNewValue(sum1ViewId), Int(45));
  // c + d = 20 + 20 = sum2 = 40
  EXPECT_EQ(engine->getNewValue(sum2), Int(40));
  // sum2 = 40 -> sum2View = min(20, 40) = 20
  EXPECT_EQ(engine->getNewValue(sum2ViewId), Int(40));
  // sum3 = sum1view + sum2view = 45 + 40 = 85
  EXPECT_EQ(engine->getNewValue(sum3), Int(85));

  for (size_t i = 0; i < 10; ++i) {
    if (i == 0) {
      EXPECT_EQ(engine->getCommittedValue(sum3viewIds[i]), Int(85));
    } else {
      EXPECT_EQ(engine->getCommittedValue(sum3viewIds[i]),
                std::max({Int(80 + i), Int(80 + i - 1), Int(85)}));
    }
  }

  engine->beginMove();
  engine->setValue(a, 5);
  engine->setValue(b, 5);
  engine->setValue(c, 5);
  engine->setValue(d, 5);
  engine->endMove();

  EXPECT_EQ(engine->getNewValue(a), Int(5));
  EXPECT_EQ(engine->getNewValue(b), Int(5));
  EXPECT_EQ(engine->getNewValue(c), Int(5));
  EXPECT_EQ(engine->getNewValue(d), Int(5));

  engine->beginCommit();
  engine->query(sum1);
  engine->query(sum2);
  engine->query(sum3);
  engine->endCommit();

  // a + b = 5 + 5 = sum1 = 10
  EXPECT_EQ(engine->getCommittedValue(sum1), Int(10));
  // c + d = 5 + 5 = sum2 = 10
  EXPECT_EQ(engine->getCommittedValue(sum2), Int(10));

  // sum1 = 10 -> sum1View = min(20, 10) = 10
  EXPECT_EQ(engine->getCommittedValue(sum1ViewId), Int(45));
  // sum2 = 10 -> sum2View = min(20, 10) = 10
  EXPECT_EQ(engine->getCommittedValue(sum2ViewId), Int(20));

  // sum3 = sum1view + sum2view = 45 + 20 = 65

  EXPECT_EQ(engine->getCommittedValue(sum3), Int(65));

  for (size_t i = 0; i < sum3viewIds.size(); ++i) {
    EXPECT_EQ(engine->getCommittedValue(sum3viewIds[i]),
              std::max({Int(80 + i), Int(80 + i - 1), Int(65)}));
  }
}

}  // namespace
