#include <gtest/gtest.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ModViewTest : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(ModViewTest, simple, (Int val)) {
  const Int denominator = *rc::gen::suchThat(rc::gen::arbitrary<Int>(),
                                             [](Int v) { return v != 0; });
  _solver->open();
  auto varId = _solver->makeIntVar(val, val, val);
  auto viewId = _solver->makeIntView<ModView>(*_solver, varId, denominator);
  _solver->close();

  RC_ASSERT(_solver->committedValue(viewId) == val % std::abs(denominator));
}
}  // namespace atlantis::testing
