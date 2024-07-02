#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ModViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(ModViewTest, simple, (Int val, Int denominator)) {
  solver->open();
  auto varId = solver->makeIntVar(val, val, val);
  auto viewId = solver->makeIntView<ModView>(*solver, varId, denominator);
  solver->close();

  RC_ASSERT(solver->committedValue(viewId) == val % std::abs(denominator));
}
}  // namespace atlantis::testing
