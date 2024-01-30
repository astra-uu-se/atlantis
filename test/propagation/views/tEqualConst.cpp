#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <memory>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/equalConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class EqualViewConst : public ::testing::Test {
 protected:
  std::unique_ptr<Solver> solver;

  void SetUp() override { solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(EqualViewConst, simple, (int a, int b)) {
  if (!solver->isOpen()) {
    solver->open();
  }
  const VarId varId = solver->makeIntVar(a, a, a);
  const VarId violationId = solver->makeIntView<EqualConst>(*solver, varId, b);
  RC_ASSERT(solver->committedValue(violationId) == std::abs(Int(a) - Int(b)));
}

RC_GTEST_FIXTURE_PROP(EqualViewConst, singleton, (int a, int b)) {
  if (!solver->isOpen()) {
    solver->open();
  }
  const VarId varId = solver->makeIntVar(a, a, a);
  const VarId violationId = solver->makeIntView<EqualConst>(*solver, varId, b);
  RC_ASSERT(solver->committedValue(violationId) == std::abs(Int(a) - Int(b)));
  RC_ASSERT(solver->lowerBound(violationId) ==
            solver->committedValue(violationId));
  RC_ASSERT(solver->upperBound(violationId) ==
            solver->committedValue(violationId));
}

RC_GTEST_FIXTURE_PROP(EqualViewConst, interval, (int a, int b)) {
  const Int size = 5;
  Int lb = Int(a) - size;
  Int ub = Int(a) + size;

  solver->open();
  const VarId varId = solver->makeIntVar(ub, lb, ub);
  const VarId violationId = solver->makeIntView<EqualConst>(*solver, varId, b);
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
    const Int expected = std::abs(val - Int(b));

    RC_ASSERT(solver->lowerBound(violationId) == violLb);
    RC_ASSERT(solver->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace atlantis::testing
