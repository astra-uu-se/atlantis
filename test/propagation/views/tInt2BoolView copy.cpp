#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/int2BoolView.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

TEST(Int2BoolViewTest, simple) {
  Solver solver;
  solver.open();
  const VarId varId = solver.makeIntVar(0, 0, 1);
  const VarId viewId = solver.makeIntView<Int2BoolView>(solver, varId);
  const std::array<Int, 5> values{0, 0, 1, 1, 0};
  solver.close();

  for (const Int committedValue : values) {
    solver.beginMove();
    solver.setValue(varId, committedValue);
    solver.endMove();
    solver.beginCommit();
    solver.query(viewId);
    solver.endCommit();
    EXPECT_EQ(solver.committedValue(viewId),
              static_cast<Int>(committedValue <= 0));

    for (const Int value : values) {
      solver.beginMove();
      solver.setValue(varId, value);
      solver.endMove();
      solver.beginProbe();
      solver.query(viewId);
      solver.endProbe();
      EXPECT_EQ(solver.currentValue(viewId), static_cast<Int>(value <= 0));
    }
  }
}

}  // namespace atlantis::testing
