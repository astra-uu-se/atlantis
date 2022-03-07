#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "constraints/equal.hpp"
#include "invariants/linear.hpp"
#include "search/assignment.hpp"

class AssignmentTest : public testing::Test {
 public:
  VarId a;
  VarId b;
  VarId c;
  VarId d;

  VarId violation;

  PropagationEngine engine;
  search::Assignment assignment { engine, violation, c };

  void SetUp() override {
    // Models the following:
    // c <- a + b
    // violation = v(c == 3)

    engine.open();
    a = engine.makeIntVar(0, 0, 10);
    b = engine.makeIntVar(0, 0, 10);
    c = engine.makeIntVar(0, 0, 10);
    d = engine.makeIntVar(3, 3, 3);
    violation = engine.makeIntVar(0, 0, 10);

    engine.makeInvariant<Linear>(std::vector<VarId>{a, b}, c);
    engine.makeConstraint<Equal>(violation, c, d);
    engine.close();
  }
};

TEST_F(AssignmentTest, assign_sets_values) {
  assignment.assign([&](auto& modifications) {
    modifications.set(a, 1);
    modifications.set(b, 2);
  });

  EXPECT_EQ(assignment.value(a), 1);
  EXPECT_EQ(assignment.value(b), 2);
}

TEST_F(AssignmentTest, satisfies_constraints) {
  EXPECT_FALSE(assignment.satisfiesConstraints());

  assignment.assign([&](auto& modifications) {
    modifications.set(a, 1);
    modifications.set(b, 2);
  });

  EXPECT_TRUE(assignment.satisfiesConstraints());
}
