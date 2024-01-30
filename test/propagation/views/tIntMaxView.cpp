#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <memory>
#include <vector>

#include "propagation/invariants/linear.hpp"
#include "propagation/solver.hpp"
#include "propagation/views/intMaxView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IntMaxViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(IntMaxViewTest, shouldAlwaysBeMax, (Int a, Int b)) {
  if (!solver->isOpen()) {
    solver->open();
  }
  const VarId varId = solver->makeIntVar(a, a, a);
  const VarId viewId = solver->makeIntView<IntMaxView>(*solver, varId, b);
  RC_ASSERT(solver->committedValue(viewId) == std::max(a, b));
}

TEST_F(IntMaxViewTest, CreateIntMaxView) {
  solver->open();

  const VarId var = solver->makeIntVar(10, 0, 10);
  const VarId viewOfVar = solver->makeIntView<IntMaxView>(*solver, var, 25);
  const VarId viewOfView =
      solver->makeIntView<IntMaxView>(*solver, viewOfVar, 50);

  EXPECT_EQ(solver->committedValue(viewOfVar), Int(25));
  EXPECT_EQ(solver->committedValue(viewOfView), Int(50));

  solver->close();
}

TEST_F(IntMaxViewTest, ComputeBounds) {
  solver->open();
  auto a = solver->makeIntVar(20, -100, 100);
  auto b = solver->makeIntVar(20, -100, 100);

  const VarId va = solver->makeIntView<IntMaxView>(*solver, a, 10);
  const VarId vb = solver->makeIntView<IntMaxView>(*solver, b, 200);

  EXPECT_EQ(solver->lowerBound(va), Int(10));
  EXPECT_EQ(solver->lowerBound(vb), Int(200));
  EXPECT_EQ(solver->upperBound(va), Int(100));
  EXPECT_EQ(solver->upperBound(vb), Int(200));

  solver->close();

  EXPECT_EQ(solver->lowerBound(va), Int(10));
  EXPECT_EQ(solver->lowerBound(vb), Int(200));
  EXPECT_EQ(solver->upperBound(va), Int(100));
  EXPECT_EQ(solver->upperBound(vb), Int(200));
}

TEST_F(IntMaxViewTest, RecomputeIntMaxView) {
  solver->open();
  const VarId a = solver->makeIntVar(20, -100, 100);
  const VarId b = solver->makeIntVar(20, -100, 100);
  const VarId sum = solver->makeIntVar(0, -100, 100);

  solver->makeInvariant<Linear>(*solver, sum, std::vector<Int>({1, 1}),
                                std::vector<VarId>({a, b}));

  const VarId viewOfVar = solver->makeIntView<IntMaxView>(*solver, sum, 10);
  const VarId viewOfView =
      solver->makeIntView<IntMaxView>(*solver, viewOfVar, 15);

  EXPECT_EQ(solver->currentValue(viewOfVar), Int(10));
  EXPECT_EQ(solver->currentValue(viewOfView), Int(15));

  solver->close();

  EXPECT_EQ(solver->currentValue(sum), Int(40));
  EXPECT_EQ(solver->currentValue(viewOfVar), Int(40));
  EXPECT_EQ(solver->currentValue(viewOfView), Int(40));

  solver->beginMove();
  solver->setValue(a, 1);
  solver->setValue(b, 1);
  solver->endMove();

  solver->beginProbe();
  solver->query(sum);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(sum), Int(2));
  EXPECT_EQ(solver->currentValue(viewOfVar), Int(10));
  EXPECT_EQ(solver->currentValue(viewOfView), Int(15));
}

TEST_F(IntMaxViewTest, PropagateIntViews) {
  solver->open();
  auto a = solver->makeIntVar(20, -100, 100);
  auto b = solver->makeIntVar(20, -100, 100);
  auto sum1 = solver->makeIntVar(0, -100, 100);
  // a + b = sum1
  auto c = solver->makeIntVar(20, -100, 100);
  auto d = solver->makeIntVar(20, -100, 100);
  auto sum2 = solver->makeIntVar(0, -100, 100);
  // c + d = sum2
  auto sum3 = solver->makeIntVar(0, -100, 100);
  // sum1 + sum2 = sum2

  solver->makeInvariant<Linear>(*solver, sum1, std::vector<Int>({1, 1}),
                                std::vector<VarId>({a, b}));

  solver->makeInvariant<Linear>(*solver, sum2, std::vector<Int>({1, 1}),
                                std::vector<VarId>({c, d}));

  const VarId sum1View = solver->makeIntView<IntMaxView>(*solver, sum1, 45);
  const VarId sum2View = solver->makeIntView<IntMaxView>(*solver, sum2, 20);

  solver->makeInvariant<Linear>(*solver, sum3, std::vector<Int>({1, 1}),
                                std::vector<VarId>({sum1View, sum2View}));

  std::vector<VarId> sum3views;
  VarId prev = sum3;
  for (size_t i = 0; i < 10; ++i) {
    sum3views.emplace_back(
        solver->makeIntView<IntMaxView>(*solver, prev, 80 + i));
    prev = sum3views[i];
  }

  EXPECT_EQ(solver->committedValue(sum1), Int(0));
  EXPECT_EQ(solver->committedValue(sum2), Int(0));
  EXPECT_EQ(solver->committedValue(sum1View), Int(45));
  EXPECT_EQ(solver->committedValue(sum2View), Int(20));

  solver->close();

  // a + b = 20 + 20 = sum1 = 40
  EXPECT_EQ(solver->currentValue(sum1), Int(40));
  // sum1 = 40 -> sum1View = min(20, 40) = 20
  EXPECT_EQ(solver->currentValue(sum1View), Int(45));
  // c + d = 20 + 20 = sum2 = 40
  EXPECT_EQ(solver->currentValue(sum2), Int(40));
  // sum2 = 40 -> sum2View = min(20, 40) = 20
  EXPECT_EQ(solver->currentValue(sum2View), Int(40));
  // sum3 = sum1view + sum2view = 45 + 40 = 85
  EXPECT_EQ(solver->currentValue(sum3), Int(85));

  for (size_t i = 0; i < 10; ++i) {
    if (i == 0) {
      EXPECT_EQ(solver->committedValue(sum3views[i]), Int(85));
    } else {
      EXPECT_EQ(solver->committedValue(sum3views[i]),
                std::max({Int(80 + i), Int(80 + i - 1), Int(85)}));
    }
  }

  solver->beginMove();
  solver->setValue(a, 5);
  solver->setValue(b, 5);
  solver->setValue(c, 5);
  solver->setValue(d, 5);
  solver->endMove();

  EXPECT_EQ(solver->currentValue(a), Int(5));
  EXPECT_EQ(solver->currentValue(b), Int(5));
  EXPECT_EQ(solver->currentValue(c), Int(5));
  EXPECT_EQ(solver->currentValue(d), Int(5));

  solver->beginCommit();
  solver->query(sum1);
  solver->query(sum2);
  solver->query(sum3);
  solver->endCommit();

  // a + b = 5 + 5 = sum1 = 10
  EXPECT_EQ(solver->committedValue(sum1), Int(10));
  // c + d = 5 + 5 = sum2 = 10
  EXPECT_EQ(solver->committedValue(sum2), Int(10));

  // sum1 = 10 -> sum1View = min(20, 10) = 10
  EXPECT_EQ(solver->committedValue(sum1View), Int(45));
  // sum2 = 10 -> sum2View = min(20, 10) = 10
  EXPECT_EQ(solver->committedValue(sum2View), Int(20));

  // sum3 = sum1view + sum2view = 45 + 20 = 65

  EXPECT_EQ(solver->committedValue(sum3), Int(65));

  for (size_t i = 0; i < sum3views.size(); ++i) {
    EXPECT_EQ(solver->committedValue(sum3views[i]),
              std::max({Int(80 + i), Int(80 + i - 1), Int(65)}));
  }
}

}  // namespace atlantis::testing
