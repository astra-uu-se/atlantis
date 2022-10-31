#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "core/propagationEngine.hpp"
#include "views/scalarView.hpp"

class ScalarViewTest : public ::testing::Test {
 protected:
  std::unique_ptr<PropagationEngine> engine;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }
};

RC_GTEST_FIXTURE_PROP(ScalarViewTest, simple, (Int a, Int b)) {
  engine->open();
  auto varId = engine->makeIntVar(a, a, a);
  auto viewId = engine->makeIntView<ScalarView>(*engine, varId, b);
  engine->close();

  RC_ASSERT(engine->committedValue(viewId) == a * b);
}
