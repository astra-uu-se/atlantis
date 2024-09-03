#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ScalarViewTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_shared<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(ScalarViewTest, simple,
                      (Int val, Int scalar, Int offset)) {
  _solver->open();
  auto varId = _solver->makeIntVar(val, val, val);
  auto viewId =
      _solver->makeIntView<ScalarView>(*_solver, varId, scalar, offset);
  _solver->close();

  RC_ASSERT(_solver->committedValue(viewId) == val * scalar + offset);
}
}  // namespace atlantis::testing
