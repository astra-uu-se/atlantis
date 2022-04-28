#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "views/notEqualView.hpp"

namespace {

class notEqualViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }
};

RC_GTEST_FIXTURE_PROP(notEqualViewTest, simple, (Int a, Int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId violationId = engine->makeIntView<NotEqualView>(varId, b);
  RC_ASSERT(engine->committedValue(violationId) == static_cast<Int>(a == b));
}

RC_GTEST_FIXTURE_PROP(notEqualViewTest, singleton, (Int a, Int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId violationId = engine->makeIntView<NotEqualView>(varId, b);
  RC_ASSERT(engine->committedValue(violationId) == static_cast<Int>(a == b));
  RC_ASSERT(engine->lowerBound(violationId) ==
            engine->committedValue(violationId));
  RC_ASSERT(engine->upperBound(violationId) ==
            engine->committedValue(violationId));
}

RC_GTEST_FIXTURE_PROP(notEqualViewTest, interval, (Int a)) {
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

  engine->open();
  const VarId varId = engine->makeIntVar(ub, lb, ub);
  const VarId violationId = engine->makeIntView<NotEqualView>(varId, b);
  engine->close();

  const Int violLb = engine->lowerBound(violationId);
  const Int violUb = engine->upperBound(violationId);

  for (Int val = lb; val <= ub; ++val) {
    engine->beginMove();
    engine->setValue(varId, val);
    engine->endMove();
    engine->beginProbe();
    engine->query(violationId);
    engine->endProbe();

    const Int actual = engine->currentValue(violationId);
    const Int expected = static_cast<Int>(val == b);

    RC_ASSERT(engine->lowerBound(violationId) == violLb);
    RC_ASSERT(engine->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace
