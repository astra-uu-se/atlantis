#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <memory>
#include <vector>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IntMaxViewTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(IntMaxViewTest, shouldAlwaysBeMax, (Int a, Int b)) {
  if (!_solver->isOpen()) {
    _solver->open();
  }
  const VarId varId = _solver->makeIntVar(a, a, a);
  const VarId viewId = _solver->makeIntView<IntMaxView>(*_solver, varId, b);
  RC_ASSERT(_solver->committedValue(viewId) == std::max(a, b));
}

TEST_F(IntMaxViewTest, CreateIntMaxView) {
  _solver->open();

  const VarId var = _solver->makeIntVar(10, 0, 10);
  const VarId viewOfVar = _solver->makeIntView<IntMaxView>(*_solver, var, 25);
  const VarId viewOfView =
      _solver->makeIntView<IntMaxView>(*_solver, viewOfVar, 50);

  EXPECT_EQ(_solver->committedValue(viewOfVar), Int(25));
  EXPECT_EQ(_solver->committedValue(viewOfView), Int(50));

  _solver->close();
}

TEST_F(IntMaxViewTest, ComputeBounds) {
  _solver->open();
  auto a = _solver->makeIntVar(20, -100, 100);
  auto b = _solver->makeIntVar(20, -100, 100);

  const VarId va = _solver->makeIntView<IntMaxView>(*_solver, a, 10);
  const VarId vb = _solver->makeIntView<IntMaxView>(*_solver, b, 200);

  EXPECT_EQ(_solver->lowerBound(va), Int(10));
  EXPECT_EQ(_solver->lowerBound(vb), Int(200));
  EXPECT_EQ(_solver->upperBound(va), Int(100));
  EXPECT_EQ(_solver->upperBound(vb), Int(200));

  _solver->close();

  EXPECT_EQ(_solver->lowerBound(va), Int(10));
  EXPECT_EQ(_solver->lowerBound(vb), Int(200));
  EXPECT_EQ(_solver->upperBound(va), Int(100));
  EXPECT_EQ(_solver->upperBound(vb), Int(200));
}

TEST_F(IntMaxViewTest, RecomputeIntMaxView) {
  _solver->open();
  const VarId a = _solver->makeIntVar(20, -100, 100);
  const VarId b = _solver->makeIntVar(20, -100, 100);
  const VarId sum = _solver->makeIntVar(0, -100, 100);

  _solver->makeInvariant<Linear>(*_solver, sum, std::vector<Int>({1, 1}),
                                 std::vector<VarId>({a, b}));

  const VarId viewOfVar = _solver->makeIntView<IntMaxView>(*_solver, sum, 10);
  const VarId viewOfView =
      _solver->makeIntView<IntMaxView>(*_solver, viewOfVar, 15);

  EXPECT_EQ(_solver->currentValue(viewOfVar), Int(10));
  EXPECT_EQ(_solver->currentValue(viewOfView), Int(15));

  _solver->close();

  EXPECT_EQ(_solver->currentValue(sum), Int(40));
  EXPECT_EQ(_solver->currentValue(viewOfVar), Int(40));
  EXPECT_EQ(_solver->currentValue(viewOfView), Int(40));

  _solver->beginMove();
  _solver->setValue(a, 1);
  _solver->setValue(b, 1);
  _solver->endMove();

  _solver->beginProbe();
  _solver->query(sum);
  _solver->endProbe();

  EXPECT_EQ(_solver->currentValue(sum), Int(2));
  EXPECT_EQ(_solver->currentValue(viewOfVar), Int(10));
  EXPECT_EQ(_solver->currentValue(viewOfView), Int(15));
}

TEST_F(IntMaxViewTest, PropagateIntViews) {
  _solver->open();
  auto a = _solver->makeIntVar(20, -100, 100);
  auto b = _solver->makeIntVar(20, -100, 100);
  auto sum1 = _solver->makeIntVar(0, -100, 100);
  // a + b = sum1
  auto c = _solver->makeIntVar(20, -100, 100);
  auto d = _solver->makeIntVar(20, -100, 100);
  auto sum2 = _solver->makeIntVar(0, -100, 100);
  // c + d = sum2
  auto sum3 = _solver->makeIntVar(0, -100, 100);
  // sum1 + sum2 = sum2

  _solver->makeInvariant<Linear>(*_solver, sum1, std::vector<Int>({1, 1}),
                                 std::vector<VarId>({a, b}));

  _solver->makeInvariant<Linear>(*_solver, sum2, std::vector<Int>({1, 1}),
                                 std::vector<VarId>({c, d}));

  const VarId sum1View = _solver->makeIntView<IntMaxView>(*_solver, sum1, 45);
  const VarId sum2View = _solver->makeIntView<IntMaxView>(*_solver, sum2, 20);

  _solver->makeInvariant<Linear>(*_solver, sum3, std::vector<Int>({1, 1}),
                                 std::vector<VarId>({sum1View, sum2View}));

  std::vector<VarId> sum3views;
  VarId prev = sum3;
  for (size_t i = 0; i < 10; ++i) {
    sum3views.emplace_back(
        _solver->makeIntView<IntMaxView>(*_solver, prev, 80 + i));
    prev = sum3views[i];
  }

  EXPECT_EQ(_solver->committedValue(sum1), Int(0));
  EXPECT_EQ(_solver->committedValue(sum2), Int(0));
  EXPECT_EQ(_solver->committedValue(sum1View), Int(45));
  EXPECT_EQ(_solver->committedValue(sum2View), Int(20));

  _solver->close();

  // a + b = 20 + 20 = sum1 = 40
  EXPECT_EQ(_solver->currentValue(sum1), Int(40));
  // sum1 = 40 -> sum1View = min(20, 40) = 20
  EXPECT_EQ(_solver->currentValue(sum1View), Int(45));
  // c + d = 20 + 20 = sum2 = 40
  EXPECT_EQ(_solver->currentValue(sum2), Int(40));
  // sum2 = 40 -> sum2View = min(20, 40) = 20
  EXPECT_EQ(_solver->currentValue(sum2View), Int(40));
  // sum3 = sum1view + sum2view = 45 + 40 = 85
  EXPECT_EQ(_solver->currentValue(sum3), Int(85));

  for (size_t i = 0; i < 10; ++i) {
    if (i == 0) {
      EXPECT_EQ(_solver->committedValue(sum3views[i]), Int(85));
    } else {
      EXPECT_EQ(_solver->committedValue(sum3views[i]),
                std::max({Int(80 + i), Int(80 + i - 1), Int(85)}));
    }
  }

  _solver->beginMove();
  _solver->setValue(a, 5);
  _solver->setValue(b, 5);
  _solver->setValue(c, 5);
  _solver->setValue(d, 5);
  _solver->endMove();

  EXPECT_EQ(_solver->currentValue(a), Int(5));
  EXPECT_EQ(_solver->currentValue(b), Int(5));
  EXPECT_EQ(_solver->currentValue(c), Int(5));
  EXPECT_EQ(_solver->currentValue(d), Int(5));

  _solver->beginCommit();
  _solver->query(sum1);
  _solver->query(sum2);
  _solver->query(sum3);
  _solver->endCommit();

  // a + b = 5 + 5 = sum1 = 10
  EXPECT_EQ(_solver->committedValue(sum1), Int(10));
  // c + d = 5 + 5 = sum2 = 10
  EXPECT_EQ(_solver->committedValue(sum2), Int(10));

  // sum1 = 10 -> sum1View = min(20, 10) = 10
  EXPECT_EQ(_solver->committedValue(sum1View), Int(45));
  // sum2 = 10 -> sum2View = min(20, 10) = 10
  EXPECT_EQ(_solver->committedValue(sum2View), Int(20));

  // sum3 = sum1view + sum2view = 45 + 20 = 65

  EXPECT_EQ(_solver->committedValue(sum3), Int(65));

  for (size_t i = 0; i < sum3views.size(); ++i) {
    EXPECT_EQ(_solver->committedValue(sum3views[i]),
              std::max({Int(80 + i), Int(80 + i - 1), Int(65)}));
  }
}

}  // namespace atlantis::testing
