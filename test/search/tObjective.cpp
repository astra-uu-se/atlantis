#include <gtest/gtest.h>

#include "propagation/propagationEngine.hpp"
#include "search/objective.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class ObjectiveTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::PropagationEngine> engine;
  propagation::VarId objectiveVarId;
  propagation::VarId totalViolationId;

  void SetUp() override {
    engine = std::make_unique<propagation::PropagationEngine>();
  }

  propagation::VarId install(
      search::Objective& objective,
      fznparser::IntSet objectiveRange = fznparser::IntSet(0, 0),
      Int initial = 0) {
    engine->open();
    objectiveVarId = engine->makeIntVar(initial, objectiveRange.lowerBound(),
                                        objectiveRange.upperBound());
    totalViolationId = engine->makeIntVar(0, 0, 0);
    auto violation = objective.registerNode(totalViolationId, objectiveVarId);
    engine->close();
    return violation;
  }
};

TEST_F(ObjectiveTest, satisfaction_objective) {
  search::Objective searchObjective(*engine, fznparser::ProblemType::SATISFY);

  auto violation = install(searchObjective);

  EXPECT_EQ(violation, totalViolationId);
  EXPECT_EQ(engine->numVariables(), 2);
  EXPECT_EQ(engine->numInvariants(), 0);
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 0);
}

TEST_F(ObjectiveTest, minimisation_objective) {
  fznparser::Model model;
  fznparser::IntSet domain(1, 10);
  fznparser::IntVar a(domain.lowerBound(), domain.upperBound(), "a");
  search::Objective searchObjective(*engine, fznparser::ProblemType::MINIMIZE);

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
  engine->setValue(objectiveVarId, 3);
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
  fznparser::Model model;
  fznparser::IntSet domain(1, 10);
  fznparser::IntVar a(domain.lowerBound(), domain.upperBound(), "a");

  search::Objective searchObjective(*engine, fznparser::ProblemType::MAXIMIZE);

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
  engine->setValue(objectiveVarId, 7);
  engine->endMove();
  engine->beginCommit();
  engine->query(violation);
  engine->endCommit();
  EXPECT_EQ(engine->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(engine->committedValue(violation), 1);
  EXPECT_EQ(engine->committedValue(*searchObjective.bound()), 8);
}

}  // namespace atlantis::testing