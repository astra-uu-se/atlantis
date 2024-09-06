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
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_shared<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(notEqualViewConst, simple, (Int a, Int b)) {
  if (!_solver->isOpen()) {
    _solver->open();
  }
  const VarViewId varId = _solver->makeIntVar(a, a, a);
  const VarViewId violationId =
      _solver->makeIntView<NotEqualConst>(*_solver, varId, b);
  RC_ASSERT(_solver->committedValue(violationId) == static_cast<Int>(a == b));
}

RC_GTEST_FIXTURE_PROP(notEqualViewConst, singleton, (Int a, Int b)) {
  if (!_solver->isOpen()) {
    _solver->open();
  }
  const VarViewId varId = _solver->makeIntVar(a, a, a);
  const VarViewId violationId =
      _solver->makeIntView<NotEqualConst>(*_solver, varId, b);
  RC_ASSERT(_solver->committedValue(violationId) == static_cast<Int>(a == b));
  RC_ASSERT(_solver->lowerBound(violationId) ==
            _solver->committedValue(violationId));
  RC_ASSERT(_solver->upperBound(violationId) ==
            _solver->committedValue(violationId));
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

  _solver->open();
  const VarViewId varId = _solver->makeIntVar(ub, lb, ub);
  const VarViewId violationId =
      _solver->makeIntView<NotEqualConst>(*_solver, varId, b);
  _solver->close();

  const Int violLb = _solver->lowerBound(violationId);
  const Int violUb = _solver->upperBound(violationId);

  for (Int val = lb; val <= ub; ++val) {
    _solver->beginMove();
    _solver->setValue(varId, val);
    _solver->endMove();
    _solver->beginProbe();
    _solver->query(violationId);
    _solver->endProbe();

    const Int actual = _solver->currentValue(violationId);
    const Int expected = static_cast<Int>(val == b);

    RC_ASSERT(_solver->lowerBound(violationId) == violLb);
    RC_ASSERT(_solver->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace atlantis::testing
