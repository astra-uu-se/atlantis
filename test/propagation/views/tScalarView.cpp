#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ScalarViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(ScalarViewTest, simple,
                      (Int val, Int scalar, Int offset)) {
  solver->open();
  auto varId = solver->makeIntVar(val, val, val);
  auto viewId = solver->makeIntView<ScalarView>(*solver, varId, scalar, offset);
  solver->close();

  RC_ASSERT(solver->committedValue(viewId) == val * scalar + offset);
}
}  // namespace atlantis::testing
