#include <gtest/gtest.h>

#include <fznparser/model.hpp>

#include "core/propagationEngine.hpp"
#include "invariantgraph/objectiveTranslator.hpp"
#include "search/objective.hpp"

class ObjectiveTest : public testing::Test {
 public:
  std::unique_ptr<PropagationEngine> engine;
  VarId objectiveVariable;
  VarId constraintViolation;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }

  search::Objective install(fznparser::FZNModel& model,
                            fznparser::IntRange objectiveRange = {0, 0},
                            Int initial = 0) {
    engine->open();
    objectiveVariable = engine->makeIntVar(initial, objectiveRange.lowerBound,
                                           objectiveRange.upperBound);
    constraintViolation = engine->makeIntVar(0, 0, 0);
    search::Objective objective = search::Objective::createAndRegister(
        *engine, getObjectiveDirection(model.objective()), constraintViolation,
        objectiveVariable);
    engine->close();
    return objective;
  }
};

TEST_F(ObjectiveTest, satisfaction_objective) {
  fznparser::FZNModel model({}, {}, {}, fznparser::Satisfy{});

  auto searchObjective = install(model);

  EXPECT_EQ(searchObjective.violation(), constraintViolation);
  EXPECT_EQ(engine->numVariables(), 2);
  EXPECT_EQ(engine->numInvariants(), 0);
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 0);
}

TEST_F(ObjectiveTest, minimisation_objective) {
  fznparser::IntRange domain{1, 10};
  fznparser::IntVariable a{"a", domain, {}, {}};
  fznparser::FZNModel model({}, {a}, {}, fznparser::Minimise{a.name});

  auto searchObjective = install(model, domain, 5);

  EXPECT_EQ(engine->lowerBound(searchObjective.bound()), 1);
  EXPECT_EQ(engine->upperBound(searchObjective.bound()), 10);
  EXPECT_EQ(engine->committedValue(searchObjective.bound()), 10);

  EXPECT_EQ(engine->numVariables(), 5);
  EXPECT_EQ(engine->numInvariants(), 2);
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 1);
  EXPECT_EQ(engine->committedValue(searchObjective.bound()), 4);

  engine->beginMove();
  engine->setValue(objectiveVariable, 3);
  engine->endMove();
  engine->beginCommit();
  engine->query(searchObjective.violation());
  engine->endCommit();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 1);
  EXPECT_EQ(engine->committedValue(searchObjective.bound()), 2);
}

TEST_F(ObjectiveTest, maximisation_objective) {
  fznparser::IntRange domain{1, 10};
  fznparser::IntVariable a{"a", domain, {}, {}};
  fznparser::FZNModel model({}, {a}, {}, fznparser::Maximise{a.name});

  auto searchObjective = install(model, domain, 5);

  EXPECT_EQ(engine->lowerBound(searchObjective.bound()), 1);
  EXPECT_EQ(engine->upperBound(searchObjective.bound()), 10);
  EXPECT_EQ(engine->committedValue(searchObjective.bound()), 1);

  EXPECT_EQ(engine->numVariables(), 5);
  EXPECT_EQ(engine->numInvariants(), 2);
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 1);
  EXPECT_EQ(engine->committedValue(searchObjective.bound()), 6);

  engine->beginMove();
  engine->setValue(objectiveVariable, 7);
  engine->endMove();
  engine->beginCommit();
  engine->query(searchObjective.violation());
  engine->endCommit();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(searchObjective.violation()), 1);
  EXPECT_EQ(engine->committedValue(searchObjective.bound()), 8);
}
