#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "search/objective.hpp"

class ObjectiveTest : public testing::Test {
 public:
  std::unique_ptr<PropagationEngine> engine;
  VarId objectiveVariable;
  VarId constraintViolation;

  void SetUp() override { engine = std::make_unique<PropagationEngine>(); }

  VarId install(search::Objective& objective,
                fznparser::IntRange objectiveRange = {0, 0}, Int initial = 0) {
    engine->open();
    objectiveVariable = engine->makeIntVar(initial, objectiveRange.lowerBound,
                                           objectiveRange.upperBound);
    constraintViolation = engine->makeIntVar(0, 0, 0);
    auto violation =
        objective.registerWithEngine(constraintViolation, objectiveVariable);
    engine->close();
    return violation;
  }
};

TEST_F(ObjectiveTest, satisfaction_objective) {
  fznparser::Model model({}, {}, {}, fznparser::Satisfy{});
  search::Objective searchObjective(*engine, model);

  auto violation = install(searchObjective);

  EXPECT_EQ(violation, constraintViolation);
  EXPECT_EQ(engine->numVariables(), 2);
  EXPECT_EQ(engine->numInvariants(), 0);
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 0);
}

TEST_F(ObjectiveTest, minimisation_objective) {
  fznparser::IntRange domain{1, 10};
  fznparser::IntVar a{"a", domain, {}, {}};
  fznparser::Model model({}, {a}, {}, fznparser::Minimise{a.name});
  search::Objective searchObjective(*engine, model);

  auto violation = install(searchObjective, domain, 5);

  EXPECT_EQ(engine->lowerBound(*searchObjective.bound()), 1);
  EXPECT_EQ(engine->upperBound(*searchObjective.bound()), 10);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 10);

  EXPECT_EQ(engine->numVariables(), 5);
  EXPECT_EQ(engine->numInvariants(), 2);
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 1);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 4);

  engine->beginMove();
  engine->setValue(objectiveVariable, 3);
  engine->endMove();
  engine->beginCommit();
  engine->query(violation);
  engine->endCommit();
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 1);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 2);
}

TEST_F(ObjectiveTest, maximisation_objective) {
  fznparser::IntRange domain{1, 10};
  fznparser::IntVar a{"a", domain, {}, {}};
  fznparser::Model model({}, {a}, {}, fznparser::Maximise{a.name});
  search::Objective searchObjective(*engine, model);

  auto violation = install(searchObjective, domain, 5);

  EXPECT_EQ(engine->lowerBound(*searchObjective.bound()), 1);
  EXPECT_EQ(engine->upperBound(*searchObjective.bound()), 10);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 1);

  EXPECT_EQ(engine->numVariables(), 5);
  EXPECT_EQ(engine->numInvariants(), 2);
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 1);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 6);

  engine->beginMove();
  engine->setValue(objectiveVariable, 7);
  engine->endMove();
  engine->beginCommit();
  engine->query(violation);
  engine->endCommit();
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 1);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 8);
}
