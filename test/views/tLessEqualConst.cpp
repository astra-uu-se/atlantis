#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "views/lessEqualConst.hpp"

namespace {

static Int computeViolation(Int a, Int b) { return std::max<Int>(0, a - b); }

class LessEqualViewConst : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }
};

RC_GTEST_FIXTURE_PROP(LessEqualViewConst, simple, (int a, int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId violationId = engine->makeIntView<LessEqualConst>(varId, b);
  RC_ASSERT(engine->committedValue(violationId) == computeViolation(a, b));
}

RC_GTEST_FIXTURE_PROP(LessEqualViewConst, singleton, (int a, int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId violationId = engine->makeIntView<LessEqualConst>(varId, b);
  RC_ASSERT(engine->committedValue(violationId) == computeViolation(a, b));
  RC_ASSERT(engine->lowerBound(violationId) ==
            engine->committedValue(violationId));
  RC_ASSERT(engine->upperBound(violationId) ==
            engine->committedValue(violationId));
}

RC_GTEST_FIXTURE_PROP(LessEqualViewConst, interval, (int a, int b)) {
  const Int size = 5;
  Int lb = Int(a) - size;
  Int ub = Int(a) + size;

  engine->open();
  const VarId varId = engine->makeIntVar(ub, lb, ub);
  const VarId violationId = engine->makeIntView<LessEqualConst>(varId, b);
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
    const Int expected = computeViolation(val, b);

    EXPECT_EQ(val <= Int(b), expected == 0);

    RC_ASSERT(engine->lowerBound(violationId) == violLb);
    RC_ASSERT(engine->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace
