#include <gtest/gtest.h>
#include <fznparser/model.hpp>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/objective.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class ObjectiveTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  propagation::VarViewId objectiveVarId{propagation::NULL_ID};
  propagation::VarViewId totalViolationVarId{propagation::NULL_ID};

  void SetUp() override { _solver = std::make_shared<propagation::Solver>(); }

  propagation::VarViewId install(
      Objective& objective,
      const fznparser::IntSet& objectiveRange = fznparser::IntSet(0, 0),
      Int initial = 0) {
    _solver->open();
    objectiveVarId = _solver->makeIntVar(initial, objectiveRange.lowerBound(),
                                         objectiveRange.upperBound());
    totalViolationVarId = _solver->makeIntVar(0, 0, 0);
    const auto violation =
        objective.registerNode(totalViolationVarId, objectiveVarId);
    _solver->close();
    return violation;
  }
};

TEST_F(ObjectiveTest, satisfaction_objective) {
  Objective searchObjective(*_solver, ObjectiveDirection::NONE);

  const auto violation = install(searchObjective);

  EXPECT_EQ(violation, totalViolationVarId);
  EXPECT_EQ(_solver->numVars(), 2);
  EXPECT_EQ(_solver->numInvariants(), 0);
  EXPECT_EQ(_solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(_solver->committedValue(violation), 0);
}

TEST_F(ObjectiveTest, minimisation_objective) {
  fznparser::Model model;
  fznparser::IntSet domain(1, 10);
  fznparser::IntVar a(domain.lowerBound(), domain.upperBound(), "a");
  Objective searchObjective(*_solver, ObjectiveDirection::MINIMIZE);

  auto violation = install(searchObjective, domain, 5);

  EXPECT_EQ(_solver->lowerBound(searchObjective.bound()), 1);
  EXPECT_EQ(_solver->upperBound(searchObjective.bound()), 10);
  EXPECT_EQ(_solver->committedValue(searchObjective.bound()), 10);

  EXPECT_EQ(_solver->numVars(), 5);
  EXPECT_EQ(_solver->numInvariants(), 2);
  EXPECT_EQ(_solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(_solver->committedValue(violation), 1);
  EXPECT_EQ(_solver->committedValue(searchObjective.bound()), 4);

  _solver->beginMove();
  _solver->setValue(objectiveVarId, 3);
  _solver->endMove();
  _solver->beginCommit();
  _solver->query(violation);
  _solver->endCommit();
  EXPECT_EQ(_solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(_solver->committedValue(violation), 1);
  EXPECT_EQ(_solver->committedValue(searchObjective.bound()), 2);
}

TEST_F(ObjectiveTest, maximisation_objective) {
  fznparser::Model model;
  fznparser::IntSet domain(1, 10);
  fznparser::IntVar a(domain.lowerBound(), domain.upperBound(), "a");

  Objective searchObjective(*_solver, ObjectiveDirection::MAXIMIZE);

  auto violation = install(searchObjective, domain, 5);

  EXPECT_EQ(_solver->lowerBound(searchObjective.bound()), 1);
  EXPECT_EQ(_solver->upperBound(searchObjective.bound()), 10);
  EXPECT_EQ(_solver->committedValue(searchObjective.bound()), 1);

  EXPECT_EQ(_solver->numVars(), 5);
  EXPECT_EQ(_solver->numInvariants(), 2);
  EXPECT_EQ(_solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(_solver->committedValue(violation), 1);
  EXPECT_EQ(_solver->committedValue(searchObjective.bound()), 6);

  _solver->beginMove();
  _solver->setValue(objectiveVarId, 7);
  _solver->endMove();
  _solver->beginCommit();
  _solver->query(violation);
  _solver->endCommit();
  EXPECT_EQ(_solver->committedValue(violation), 0);

  searchObjective.tighten();
  EXPECT_EQ(_solver->committedValue(violation), 1);
  EXPECT_EQ(_solver->committedValue(searchObjective.bound()), 8);
}

TEST_F(ObjectiveTest, minimisation_objective_handles_extreme_bounds) {
  Objective searchObjective(*_solver, ObjectiveDirection::MINIMIZE);

  auto violation =
      install(searchObjective,
              fznparser::IntSet(std::numeric_limits<Int>::min() + 14,
                                std::numeric_limits<Int>::max()),
              0);

  EXPECT_EQ(_solver->lowerBound(violation), 0);
  EXPECT_EQ(_solver->upperBound(violation), std::numeric_limits<Int>::max());
}

}  // namespace atlantis::testing
