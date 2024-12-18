#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/searchVariable.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class AssignmentTest : public ::testing::Test {
 public:
  propagation::VarId a{propagation::NULL_ID};
  propagation::VarId b{propagation::NULL_ID};
  propagation::VarId c{propagation::NULL_ID};
  propagation::VarId d{propagation::NULL_ID};
  propagation::VarViewId violation{propagation::NULL_ID};

  std::shared_ptr<MockNeighborhood> _neighborhood;

  std::shared_ptr<propagation::Solver> _solver;

  // Models the following simple COP:
  // c <- a + b (a and b have domain 0..10)
  // violation = v(c == 3)
  // obj: minimise(a)
  void SetUp() override {
    _solver = std::make_shared<propagation::Solver>();

    _solver->open();
    a = propagation::VarId(_solver->makeIntVar(0, 0, 10));
    b = propagation::VarId(_solver->makeIntVar(0, 0, 10));
    c = propagation::VarId(_solver->makeIntVar(0, 0, 10));
    d = propagation::VarId(_solver->makeIntVar(3, 3, 3));
    violation = _solver->makeIntVar(0, 0, 10);

    _solver->makeInvariant<propagation::Linear>(
        *_solver, c,
        std::vector<propagation::VarViewId>{propagation::VarViewId{a},
                                            propagation::VarViewId{b}});
    _solver->makeViolationInvariant<propagation::Equal>(*_solver, violation, c,
                                                        d);
    _solver->close();

    _neighborhood = std::make_shared<MockNeighborhood>();
  }
};

TEST_F(AssignmentTest, search_vars_are_identified) {
  const Assignment assignment(*_solver, *_neighborhood, violation, a,
                              ObjectiveDirection::MINIMIZE,
                              _solver->lowerBound(a));

  const std::vector<propagation::VarId> expectedSearchVars{a, b, d};
  EXPECT_EQ(assignment.searchVars(), expectedSearchVars);
}

TEST_F(AssignmentTest, assign_sets_values) {
  Assignment assignment(*_solver, *_neighborhood, violation, a,
                        ObjectiveDirection::MINIMIZE, _solver->lowerBound(a));

  assignment.set(a, 1);
  assignment.set(b, 2);

  EXPECT_EQ(assignment.currentValue(a), 1);
  EXPECT_EQ(assignment.currentValue(b), 2);
}

TEST_F(AssignmentTest, satisfies_constraints) {
  Assignment assignment(*_solver, *_neighborhood, violation, a,
                        ObjectiveDirection::MINIMIZE, _solver->lowerBound(a));

  EXPECT_FALSE(assignment.satisfiesConstraints());

  _solver->beginMove();
  assignment.set(a, 1);
  assignment.set(b, 2);
  _solver->endMove();
  _solver->beginCommit();
  _solver->endCommit();

  EXPECT_TRUE(assignment.satisfiesConstraints());
}

TEST_F(AssignmentTest, initialize) {
  std::vector<SearchVar> vars{
      SearchVar(propagation::NULL_ID, std::make_shared<SearchDomain>(0, 10))};

  RandomProvider random{123456};

  Assignment assignment(*_solver, *_neighborhood, violation, a,
                        ObjectiveDirection::MINIMIZE, _solver->lowerBound(a));

  EXPECT_CALL(*_neighborhood, coveredVars()).WillRepeatedly(ReturnRef(vars));
  EXPECT_CALL(*_neighborhood, initialize(Ref(random), Ref(assignment)))
      .Times(1);

  assignment.initialize(random);
}

}  // namespace atlantis::testing
