#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <limits>
#include <memory>
#include <random>
#include <vector>

#include "propagation/propagationEngine.hpp"
#include "propagation/views/equalConst.hpp"
#include "types.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class EqualViewConst : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }
};

RC_GTEST_FIXTURE_PROP(EqualViewConst, simple, (int a, int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId violationId = engine->makeIntView<EqualConst>(*engine, varId, b);
  RC_ASSERT(engine->committedValue(violationId) == std::abs(Int(a) - Int(b)));
}

RC_GTEST_FIXTURE_PROP(EqualViewConst, singleton, (int a, int b)) {
  if (!engine->isOpen()) {
    engine->open();
  }
  const VarId varId = engine->makeIntVar(a, a, a);
  const VarId violationId = engine->makeIntView<EqualConst>(*engine, varId, b);
  RC_ASSERT(engine->committedValue(violationId) == std::abs(Int(a) - Int(b)));
  RC_ASSERT(engine->lowerBound(violationId) ==
            engine->committedValue(violationId));
  RC_ASSERT(engine->upperBound(violationId) ==
            engine->committedValue(violationId));
}

RC_GTEST_FIXTURE_PROP(EqualViewConst, interval, (int a, int b)) {
  const Int size = 5;
  Int lb = Int(a) - size;
  Int ub = Int(a) + size;

  engine->open();
  const VarId varId = engine->makeIntVar(ub, lb, ub);
  const VarId violationId = engine->makeIntView<EqualConst>(*engine, varId, b);
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
    const Int expected = std::abs(val - Int(b));

    RC_ASSERT(engine->lowerBound(violationId) == violLb);
    RC_ASSERT(engine->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace atlantis::testing
