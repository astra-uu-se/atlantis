#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/objective.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class ObjectiveTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::Solver> solver;
  propagation::VarId objectiveVarId;
  propagation::VarId totalViolationVarId;

  void SetUp() override { solver = std::make_unique<propagation::Solver>(); }

  propagation::VarId install(
      search::Objective& objective,
      const fznparser::IntSet& objectiveRange = fznparser::IntSet(0, 0),
      Int initial = 0) {
    solver->open();
    objectiveVarId = solver->makeIntVar(initial, objectiveRange.lowerBound(),
                                        objectiveRange.upperBound());
    totalViolationVarId = solver->makeIntVar(0, 0, 0);
    auto violation =
        objective.registerNode(totalViolationVarId, objectiveVarId);
    solver->close();
    return violation;
  }
};

TEST_F(ObjectiveTest, satisfaction_objective) {
  search::Objective searchObjective(*solver, fznparser::ProblemType::SATISFY);

  auto violation = install(searchObjective);

  EXPECT_EQ(violation, totalViolationVarId);
  EXPECT_EQ(solver->numVars(), 2);
  EXPECT_EQ(solver->numInvariants(), 0);
  EXPECT_EQ(solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(solver->committedValue(violation), 0);
}

TEST_F(ObjectiveTest, minimisation_objective) {
  fznparser::Model model;
  fznparser::IntSet domain(1, 10);
  fznparser::IntVar a(domain.lowerBound(), domain.upperBound(), "a");
  search::Objective searchObjective(*solver, fznparser::ProblemType::MINIMIZE);

  auto violation = install(searchObjective, domain, 5);

  EXPECT_EQ(solver->lowerBound(*searchObjective.bound()), 1);
  EXPECT_EQ(solver->upperBound(*searchObjective.bound()), 10);
  EXPECT_EQ(solver->committedValue(*searchObjective.bound()), 10);

  EXPECT_EQ(solver->numVars(), 5);
  EXPECT_EQ(solver->numInvariants(), 2);
  EXPECT_EQ(solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(solver->committedValue(violation), 1);
  EXPECT_EQ(solver->committedValue(*searchObjective.bound()), 4);

  solver->beginMove();
  solver->setValue(objectiveVarId, 3);
  solver->endMove();
  solver->beginCommit();
  solver->query(violation);
  solver->endCommit();
  EXPECT_EQ(solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(solver->committedValue(violation), 1);
  EXPECT_EQ(solver->committedValue(*searchObjective.bound()), 2);
}

TEST_F(ObjectiveTest, maximisation_objective) {
  fznparser::Model model;
  fznparser::IntSet domain(1, 10);
  fznparser::IntVar a(domain.lowerBound(), domain.upperBound(), "a");

  search::Objective searchObjective(*solver, fznparser::ProblemType::MAXIMIZE);

  auto violation = install(searchObjective, domain, 5);

  EXPECT_EQ(solver->lowerBound(*searchObjective.bound()), 1);
  EXPECT_EQ(solver->upperBound(*searchObjective.bound()), 10);
  EXPECT_EQ(solver->committedValue(*searchObjective.bound()), 1);

  EXPECT_EQ(solver->numVars(), 5);
  EXPECT_EQ(solver->numInvariants(), 2);
  EXPECT_EQ(solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(solver->committedValue(violation), 1);
  EXPECT_EQ(solver->committedValue(*searchObjective.bound()), 6);

  solver->beginMove();
  solver->setValue(objectiveVarId, 7);
  solver->endMove();
  solver->beginCommit();
  solver->query(violation);
  solver->endCommit();
  EXPECT_EQ(solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(solver->committedValue(violation), 1);
  EXPECT_EQ(solver->committedValue(*searchObjective.bound()), 8);
}

}  // namespace atlantis::testing
