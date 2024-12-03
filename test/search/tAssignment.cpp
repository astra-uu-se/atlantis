#include <gtest/gtest.h>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/search/assignment.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AssignmentTest : public ::testing::Test {
 public:
  propagation::VarId a{propagation::NULL_ID};
  propagation::VarId b{propagation::NULL_ID};
  propagation::VarId c{propagation::NULL_ID};
  propagation::VarId d{propagation::NULL_ID};

  propagation::VarViewId violation{propagation::NULL_ID};

  propagation::Solver solver;

  // Models the following simple COP:
  // c <- a + b (a and b have domain 0..10)
  // violation = v(c == 3)
  // obj: minimise(a)
  void SetUp() override {
    solver.open();
    a = propagation::VarId(solver.makeIntVar(0, 0, 10));
    b = propagation::VarId(solver.makeIntVar(0, 0, 10));
    c = propagation::VarId(solver.makeIntVar(0, 0, 10));
    d = propagation::VarId(solver.makeIntVar(3, 3, 3));
    violation = solver.makeIntVar(0, 0, 10);

    solver.makeInvariant<propagation::Linear>(
        solver, c,
        std::vector<propagation::VarViewId>{propagation::VarViewId{a},
                                            propagation::VarViewId{b}});
    solver.makeViolationInvariant<propagation::Equal>(solver, violation, c, d);
    solver.close();
  }
};

TEST_F(AssignmentTest, search_vars_are_identified) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  std::vector<propagation::VarId> expectedSearchVars{a, b, d};
  EXPECT_EQ(assignment.searchVars(), expectedSearchVars);
}

TEST_F(AssignmentTest, cost) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  EXPECT_FALSE(assignment.cost().satisfiesConstraints());

  // c has value 0, which is 3 away from 3.
  EXPECT_EQ(assignment.cost().evaluate(1, 1), 3);

  assignment.assign([&]([[maybe_unused]] auto& modifications) {
    assignment.set(a, 2);
    assignment.set(b, 1);
  });

  EXPECT_TRUE(assignment.cost().satisfiesConstraints());

  // no violation and a (the objective) has value 2.
  EXPECT_EQ(assignment.cost().evaluate(1, 1), 2);
}

TEST_F(AssignmentTest, assign_sets_values) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  assignment.assign([&]([[maybe_unused]] auto& modifications) {
    assignment.set(a, 1);
    assignment.set(b, 2);
  });

  EXPECT_EQ(assignment.value(a), 1);
  EXPECT_EQ(assignment.value(b), 2);
}

TEST_F(AssignmentTest, probe) {
  search::Assignment assignment{solver, violation, a,
                                propagation::ObjectiveDirection::MINIMIZE,
                                solver.lowerBound(a)};

  auto cost = assignment.probe([&]([[maybe_unused]] auto& modifications) {
    assignment.set(a, 1);
    assignment.set(b, 2);
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

  assignment.assign([&]([[maybe_unused]] auto& modifications) {
    assignment.set(a, 1);
    assignment.set(b, 2);
  });

  EXPECT_TRUE(assignment.satisfiesConstraints());
}

}  // namespace atlantis::testing
