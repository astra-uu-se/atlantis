#include <gtest/gtest.h>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/search/assignment.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AssignmentTest : public ::testing::Test {
 public:
  propagation::VarId a;
  propagation::VarId b;
  propagation::VarId c;
  propagation::VarId d;

  propagation::VarId violation;

  propagation::Solver solver;

  // Models the following simple COP:
  // c <- a + b (a and b have domain 0..10)
  // violation = v(c == 3)
  // obj: minimise(a)
  void SetUp() override {
    solver.open();
    a = solver.makeIntVar(0, 0, 10);
    b = solver.makeIntVar(0, 0, 10);
    c = solver.makeIntVar(0, 0, 10);
    d = solver.makeIntVar(3, 3, 3);
    violation = solver.makeIntVar(0, 0, 10);

    solver.makeInvariant<propagation::Linear>(
        solver, c, std::vector<propagation::VarId>{a, b});
    solver.makeViolationInvariant<propagation::Equal>(solver, violation, c, d);
    solver.close();
  }
};

TEST_F(AssignmentTest, search_vars_are_identified) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  std::vector<propagation::VarId> expectedSearchVars{a, b};
  EXPECT_EQ(assignment.searchVars(), expectedSearchVars);
}

TEST_F(AssignmentTest, cost) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  EXPECT_FALSE(assignment.cost().satisfiesConstraints());

  // c has value 0, which is 3 away from 3.
  EXPECT_EQ(assignment.cost().evaluate(1, 1), 3);

  assignment.assign([&](auto& modifications) {
    modifications.set(a, 2);
    modifications.set(b, 1);
  });

  EXPECT_TRUE(assignment.cost().satisfiesConstraints());

  // no violation and a (the objective) has value 2.
  EXPECT_EQ(assignment.cost().evaluate(1, 1), 2);
}

TEST_F(AssignmentTest, assign_sets_values) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  assignment.assign([&](auto& modifications) {
    modifications.set(a, 1);
    modifications.set(b, 2);
  });

  EXPECT_EQ(assignment.value(a), 1);
  EXPECT_EQ(assignment.value(b), 2);
}

TEST_F(AssignmentTest, probe) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  auto cost = assignment.probe([&](auto& modifications) {
    modifications.set(a, 1);
    modifications.set(b, 2);
  });

  EXPECT_FALSE(assignment.cost().satisfiesConstraints());
  EXPECT_EQ(assignment.cost().evaluate(1, 1), 3);

  EXPECT_TRUE(cost.satisfiesConstraints());
  EXPECT_EQ(cost.evaluate(1, 1), 1);
}

TEST_F(AssignmentTest, satisfies_constraints) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  EXPECT_FALSE(assignment.satisfiesConstraints());

  assignment.assign([&](auto& modifications) {
    modifications.set(a, 1);
    modifications.set(b, 2);
  });

  EXPECT_TRUE(assignment.satisfiesConstraints());
}

}  // namespace atlantis::testing
