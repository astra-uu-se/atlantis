#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class notEqualViewConst : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(notEqualViewConst, simple, (Int a, Int b)) {
  if (!solver->isOpen()) {
    solver->open();
  }
  const VarId varId = solver->makeIntVar(a, a, a);
  const VarId violationId =
      solver->makeIntView<NotEqualConst>(*solver, varId, b);
  RC_ASSERT(solver->committedValue(violationId) == static_cast<Int>(a == b));
}

RC_GTEST_FIXTURE_PROP(notEqualViewConst, singleton, (Int a, Int b)) {
  if (!solver->isOpen()) {
    solver->open();
  }
  const VarId varId = solver->makeIntVar(a, a, a);
  const VarId violationId =
      solver->makeIntView<NotEqualConst>(*solver, varId, b);
  RC_ASSERT(solver->committedValue(violationId) == static_cast<Int>(a == b));
  RC_ASSERT(solver->lowerBound(violationId) ==
            solver->committedValue(violationId));
  RC_ASSERT(solver->upperBound(violationId) ==
            solver->committedValue(violationId));
}

RC_GTEST_FIXTURE_PROP(notEqualViewConst, interval, (Int a)) {
  const Int size = 5;
  Int lb, ub;
  if ((a > 0) && (size > std::numeric_limits<Int>::max() - a)) {
    lb = std::numeric_limits<Int>::max() - 2 * size;
    ub = std::numeric_limits<Int>::max();
  } else if ((a < 0) && (size < std::numeric_limits<Int>::max() - a)) {
    lb = std::numeric_limits<Int>::min();
    ub = std::numeric_limits<Int>::min() + 2 * size;
  } else {
    lb = a - size;
    ub = a + size;
  }

  const Int b = lb + size;

  solver->open();
  const VarId varId = solver->makeIntVar(ub, lb, ub);
  const VarId violationId =
      solver->makeIntView<NotEqualConst>(*solver, varId, b);
  solver->close();

  const Int violLb = solver->lowerBound(violationId);
  const Int violUb = solver->upperBound(violationId);

  for (Int val = lb; val <= ub; ++val) {
    solver->beginMove();
    solver->setValue(varId, val);
    solver->endMove();
    solver->beginProbe();
    solver->query(violationId);
    solver->endProbe();

    const Int actual = solver->currentValue(violationId);
    const Int expected = static_cast<Int>(val == b);

    RC_ASSERT(solver->lowerBound(violationId) == violLb);
    RC_ASSERT(solver->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace atlantis::testing
