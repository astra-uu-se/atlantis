#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "propagation/solver.hpp"
#include "propagation/views/scalarView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ScalarViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(ScalarViewTest, simple, (Int a, Int b)) {
  solver->open();
  auto varId = solver->makeIntVar(a, a, a);
  auto viewId = solver->makeIntView<ScalarView>(*solver, varId, b);
  solver->close();

  RC_ASSERT(solver->committedValue(viewId) == a * b);
}
}  // namespace atlantis::testing