#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class IntOffsetViewTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_shared<Solver>(); }
};

TEST_F(IntOffsetViewTest, BoundsSaturateInsteadOfOverflowing) {
  _solver->open();
  const auto varId = _solver->makeIntVar(
      overflow::kIntMax - 150, overflow::kIntMax - 200,
      overflow::kIntMax - 100);
  const auto viewId =
      _solver->makeIntView<IntOffsetView>(*_solver, varId, 150);
  _solver->close();

  EXPECT_EQ(_solver->lowerBound(viewId), overflow::kIntMax - 50);
  EXPECT_EQ(_solver->upperBound(viewId), overflow::kIntMax);
  EXPECT_EQ(_solver->committedValue(viewId), overflow::kIntMax);
}

}  // namespace atlantis::testing
