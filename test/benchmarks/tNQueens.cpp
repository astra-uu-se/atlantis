#include <gtest/gtest.h>

#include "propagation/constraints/allDifferent.hpp"
#include "propagation/invariants/linear.hpp"
#include "propagation/solver.hpp"
#include "propagation/views/intOffsetView.hpp"

namespace atlantis::testing {

TEST(NQueens, CommitsInvariant) {
  propagation::Solver solver;

  std::vector<propagation::VarId> queens;
  std::vector<propagation::VarId> q_offset_plus;
  std::vector<propagation::VarId> q_offset_minus;

  const int n = 4;

  solver.open();
  for (Int i = 0; i < n; ++i) {
    const propagation::VarId q = solver.makeIntVar(1, 1, n);
    queens.push_back(q);
    q_offset_minus.push_back(
        solver.makeIntView<propagation::IntOffsetView>(solver, q, -i));
    q_offset_plus.push_back(
        solver.makeIntView<propagation::IntOffsetView>(solver, q, i));
  }

  auto violation1 = solver.makeIntVar(0, 0, n);
  auto violation2 = solver.makeIntVar(0, 0, n);
  auto violation3 = solver.makeIntVar(0, 0, n);

  solver.makeConstraint<propagation::AllDifferent>(solver, violation1, queens);
  solver.makeConstraint<propagation::AllDifferent>(solver, violation2,
                                                   q_offset_minus);
  solver.makeConstraint<propagation::AllDifferent>(solver, violation3,
                                                   q_offset_plus);

  auto total_violation = solver.makeIntVar(0, 0, 3 * n);

  solver.makeInvariant<propagation::Linear>(
      solver, total_violation,
      std::vector<propagation::VarId>{violation1, violation2, violation3});

  solver.close();

  solver.beginMove();
  std::vector<Int> initial{1, 4, 3, 3};
  for (size_t i = 0; i < queens.size(); i++)
    solver.setValue(queens[i], initial[i]);
  solver.endMove();

  solver.beginCommit();
  solver.query(total_violation);
  solver.endCommit();

  EXPECT_EQ(solver.committedValue(violation1), 1);       // OK!
  EXPECT_EQ(solver.committedValue(violation2), 1);       // OK!
  EXPECT_EQ(solver.committedValue(violation3), 1);       // OK!
  EXPECT_EQ(solver.committedValue(total_violation), 3);  // OK!

  solver.beginMove();
  solver.setValue(queens[0], 3);
  solver.endMove();

  solver.beginCommit();
  solver.query(total_violation);
  solver.endCommit();

  EXPECT_EQ(solver.committedValue(violation1), 2);  // OK!
  EXPECT_EQ(solver.committedValue(violation2), 1);  // OK!
  EXPECT_EQ(solver.committedValue(violation3), 1);  // OK!
  EXPECT_EQ(solver.committedValue(total_violation),
            4);  // Fail - Actual value: 2
}

}  // namespace atlantis::testing