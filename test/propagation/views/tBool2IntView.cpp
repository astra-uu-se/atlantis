#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "propagation/solver.hpp"
#include "propagation/views/bool2IntView.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

TEST(Bool2IntViewTest, simple) {
  Solver solver;
  solver.open();
  const VarId varId = solver.makeIntVar(0, 0, 1);
  const VarId viewId = solver.makeIntView<Bool2IntView>(solver, varId);
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
              static_cast<Int>(committedValue == 0));

    for (const Int value : values) {
      solver.beginMove();
      solver.setValue(varId, value);
      solver.endMove();
      solver.beginProbe();
      solver.query(viewId);
      solver.endProbe();
      EXPECT_EQ(solver.currentValue(viewId), static_cast<Int>(value == 0));
    }
  }
}

}  // namespace atlantis::testing
