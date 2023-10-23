#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "propagation/propagationEngine.hpp"
#include "propagation/views/bool2IntView.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

TEST(Bool2IntViewTest, simple) {
  PropagationEngine engine;
  engine.open();
  const VarId varId = engine.makeIntVar(0, 0, 1);
  const VarId viewId = engine.makeIntView<Bool2IntView>(engine, varId);
  const std::array<Int, 5> values{0, 0, 1, 1, 0};
  engine.close();

  for (const Int committedValue : values) {
    engine.beginMove();
    engine.setValue(varId, committedValue);
    engine.endMove();
    engine.beginCommit();
    engine.query(viewId);
    engine.endCommit();
    EXPECT_EQ(engine.committedValue(viewId),
              static_cast<Int>(committedValue == 0));

    for (const Int value : values) {
      engine.beginMove();
      engine.setValue(varId, value);
      engine.endMove();
      engine.beginProbe();
      engine.query(viewId);
      engine.endProbe();
      EXPECT_EQ(engine.currentValue(viewId), static_cast<Int>(value == 0));
    }
  }
}

}  // namespace atlantis::testing
