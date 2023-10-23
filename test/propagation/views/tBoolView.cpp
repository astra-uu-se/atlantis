#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/violation2BoolView.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class BoolViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

TEST_F(BoolViewTest, CreateBoolView) {
  solver->open();

  const VarId var = solver->makeIntVar(10, 0, 10);
  auto viewOfVar = solver->makeIntView<Violation2BoolView>(*solver, var);
  auto viewOfView = solver->makeIntView<Violation2BoolView>(*solver, viewOfVar);

  EXPECT_EQ(solver->committedValue(viewOfVar), Int(1));
  EXPECT_EQ(solver->committedValue(viewOfView), Int(1));

  solver->close();
}

TEST_F(BoolViewTest, ComputeBounds) {
  solver->open();
  auto a = solver->makeIntVar(20, -100, 100);

  auto va = solver->makeIntView<Violation2BoolView>(*solver, a);

  EXPECT_EQ(solver->lowerBound(va), Int(0));
  EXPECT_EQ(solver->upperBound(va), Int(1));

  solver->close();

  EXPECT_EQ(solver->lowerBound(va), Int(0));
  EXPECT_EQ(solver->upperBound(va), Int(1));
}

TEST_F(BoolViewTest, RecomputeBoolView) {
  solver->open();
  auto a = solver->makeIntVar(20, -100, 100);

  auto viewOfVarId = solver->makeIntView<Violation2BoolView>(*solver, a);

  EXPECT_EQ(solver->currentValue(viewOfVarId), Int(1));

  solver->close();

  EXPECT_EQ(solver->currentValue(viewOfVarId), Int(1));

  solver->beginMove();
  solver->setValue(a, 0);
  solver->endMove();

  solver->beginProbe();
  solver->query(a);
  solver->endProbe();

  EXPECT_EQ(solver->currentValue(viewOfVarId), Int(0));
}

}  // namespace atlantis::testing
