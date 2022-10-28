#include <gtest/gtest.h>

#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "invariants/linear.hpp"
#include "views/intOffsetView.hpp"

TEST(NQueens, CommitsInvariant) {
  PropagationEngine engine;

  std::vector<VarId> queens;
  std::vector<VarId> q_offset_plus;
  std::vector<VarId> q_offset_minus;

  const int n = 4;

  engine.open();
  for (Int i = 0; i < n; ++i) {
    const VarId q = engine.makeIntVar(1, 1, n);
    queens.push_back(q);
    q_offset_minus.push_back(engine.makeIntView<IntOffsetView>(engine, q, -i));
    q_offset_plus.push_back(engine.makeIntView<IntOffsetView>(engine, q, i));
  }

  auto violation1 = engine.makeIntVar(0, 0, n);
  auto violation2 = engine.makeIntVar(0, 0, n);
  auto violation3 = engine.makeIntVar(0, 0, n);

  engine.makeConstraint<AllDifferent>(engine, violation1, queens);
  engine.makeConstraint<AllDifferent>(engine, violation2, q_offset_minus);
  engine.makeConstraint<AllDifferent>(engine, violation3, q_offset_plus);

  auto total_violation = engine.makeIntVar(0, 0, 3 * n);

  engine.makeInvariant<Linear>(
      engine, total_violation,
      std::vector<VarId>{violation1, violation2, violation3});

  engine.close();

  engine.beginMove();
  std::vector<Int> initial{1, 4, 3, 3};
  for (size_t i = 0; i < queens.size(); i++)
    engine.setValue(queens[i], initial[i]);
  engine.endMove();

  engine.beginCommit();
  engine.query(total_violation);
  engine.endCommit();

  EXPECT_EQ(engine.committedValue(violation1), 1);       // OK!
  EXPECT_EQ(engine.committedValue(violation2), 1);       // OK!
  EXPECT_EQ(engine.committedValue(violation3), 1);       // OK!
  EXPECT_EQ(engine.committedValue(total_violation), 3);  // OK!

  engine.beginMove();
  engine.setValue(queens[0], 3);
  engine.endMove();

  engine.beginCommit();
  engine.query(total_violation);
  engine.endCommit();

  EXPECT_EQ(engine.committedValue(violation1), 2);  // OK!
  EXPECT_EQ(engine.committedValue(violation2), 1);  // OK!
  EXPECT_EQ(engine.committedValue(violation3), 1);  // OK!
  EXPECT_EQ(engine.committedValue(total_violation),
            4);  // Fail - Actual value: 2
}