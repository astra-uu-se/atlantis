#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/violation2BoolView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolViewTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_unique<Solver>(); }
};

TEST_F(BoolViewTest, CreateBoolView) {
  _solver->open();

  const VarId var = _solver->makeIntVar(10, 0, 10);
  auto viewOfVar = _solver->makeIntView<Violation2BoolView>(*_solver, var);
  auto viewOfView =
      _solver->makeIntView<Violation2BoolView>(*_solver, viewOfVar);

  EXPECT_EQ(_solver->committedValue(viewOfVar), Int(1));
  EXPECT_EQ(_solver->committedValue(viewOfView), Int(1));

  _solver->close();
}

TEST_F(BoolViewTest, ComputeBounds) {
  _solver->open();
  auto a = _solver->makeIntVar(20, -100, 100);

  auto va = _solver->makeIntView<Violation2BoolView>(*_solver, a);

  EXPECT_EQ(_solver->lowerBound(va), Int(0));
  EXPECT_EQ(_solver->upperBound(va), Int(1));

  _solver->close();

  EXPECT_EQ(_solver->lowerBound(va), Int(0));
  EXPECT_EQ(_solver->upperBound(va), Int(1));
}

TEST_F(BoolViewTest, RecomputeBoolView) {
  _solver->open();
  auto a = _solver->makeIntVar(20, -100, 100);

  auto viewOfVarId = _solver->makeIntView<Violation2BoolView>(*_solver, a);

  EXPECT_EQ(_solver->currentValue(viewOfVarId), Int(1));

  _solver->close();

  EXPECT_EQ(_solver->currentValue(viewOfVarId), Int(1));

  _solver->beginMove();
  _solver->setValue(a, 0);
  _solver->endMove();

  _solver->beginProbe();
  _solver->query(a);
  _solver->endProbe();

  EXPECT_EQ(_solver->currentValue(viewOfVarId), Int(0));
}

}  // namespace atlantis::testing
