#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/cost.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

class CostTest : public ::testing::Test {
public:
  static std::vector<Cost> generateCosts() {
    return std::vector<Cost>{
      Cost(),
      Cost(5),
      Cost(false, ObjectiveDirection::NONE),
      Cost(false, ObjectiveDirection::MINIMIZE),
      Cost(false, ObjectiveDirection::MAXIMIZE),
      Cost(true, ObjectiveDirection::NONE),
      Cost(true, ObjectiveDirection::MINIMIZE),
      Cost(true, ObjectiveDirection::MAXIMIZE),
      Cost(1, true),
      Cost(1, false),
      Cost(5, 1, true),
      Cost(5, 1, false)};
  }
  static Int getObj(const Assignment& assignment) {
    switch (assignment.objectiveDirection()) {
      case ObjectiveDirection::MINIMIZE:
        return assignment.currentObjective();
      case ObjectiveDirection::MAXIMIZE:
        return -assignment.currentObjective();
      case ObjectiveDirection::NONE:
      default:
        return 0;
    }
  }
};

TEST_F(CostTest, constructor) {
  const auto costs = generateCosts();
  std::vector<std::optional<Int>> violation{
    std::nullopt,
    {5},
    std::nullopt, std::nullopt, std::nullopt,
    {std::numeric_limits<Int>::max()}, {std::numeric_limits<Int>::max()}, {std::numeric_limits<Int>::max()},
    std::nullopt, std::nullopt,
    {5}, {5}};

  std::vector<std::optional<Int>> objective{
    std::nullopt,
    std::nullopt,
    std::nullopt, {std::numeric_limits<Int>::max()}, {std::numeric_limits<Int>::max()},
    std::nullopt, {std::numeric_limits<Int>::max()}, {std::numeric_limits<Int>::max()},
    {1}, {-1},
    {1}, {-1}};

  EXPECT_EQ(costs.size(), violation.size());
  EXPECT_EQ(costs.size(), objective.size());

  for (size_t i = 0; i < costs.size(); i++) {
    EXPECT_EQ(costs[i].hasViolation(), violation[i].has_value()) << "iteration: " << i;
    EXPECT_EQ(costs[i].hasObjective(), objective[i].has_value()) << "iteration: " << i;
    EXPECT_EQ(costs[i].violation(), violation[i].value_or(Int{0})) << "iteration: " << i;
    EXPECT_EQ(costs[i].objective(), objective[i].value_or(Int{0})) << "iteration: " << i;
    if (violation[i].has_value()) {
      EXPECT_EQ(costs[i].satisfiesConstraints(), violation[i].value() == 0);
    } else {
      EXPECT_TRUE(costs[i].satisfiesConstraints());
    }
  }
}

TEST_F(CostTest, assignment) {
  propagation::Solver solver;
  solver.open();
  const auto violVar = solver.makeIntVar(1, 0, 10);
  const auto objVar = solver.makeIntVar(5, 2, 10);
  solver.close();
  std::shared_ptr<MockNeighborhood> neighborhood = std::make_shared<MockNeighborhood>();

  std::vector<Assignment> assignments{
    Assignment(solver, neighborhood, propagation::NULL_ID, propagation::NULL_ID, ObjectiveDirection::NONE, 0),
    Assignment(solver, neighborhood, propagation::NULL_ID, propagation::NULL_ID, ObjectiveDirection::MINIMIZE, 2),
    Assignment(solver, neighborhood, propagation::NULL_ID, propagation::NULL_ID, ObjectiveDirection::MAXIMIZE, 10),
    Assignment(solver, neighborhood, violVar, propagation::NULL_ID, ObjectiveDirection::NONE, 0),
    Assignment(solver, neighborhood, violVar, propagation::NULL_ID, ObjectiveDirection::MINIMIZE, 2),
    Assignment(solver, neighborhood, violVar, propagation::NULL_ID, ObjectiveDirection::MAXIMIZE, 10),
    Assignment(solver, neighborhood, propagation::NULL_ID, objVar, ObjectiveDirection::NONE, 0),
    Assignment(solver, neighborhood, propagation::NULL_ID, objVar, ObjectiveDirection::MINIMIZE, 2),
    Assignment(solver, neighborhood, propagation::NULL_ID, objVar, ObjectiveDirection::MAXIMIZE, 10),
    Assignment(solver, neighborhood, violVar, objVar, ObjectiveDirection::NONE, 0),
    Assignment(solver, neighborhood, violVar, objVar, ObjectiveDirection::MINIMIZE, 2),
    Assignment(solver, neighborhood, violVar, objVar, ObjectiveDirection::MAXIMIZE, 10),
    };

  for (size_t i = 0; i < assignments.size(); i++) {
    const Cost cost(assignments[i]);
    EXPECT_EQ(cost.hasViolation(), assignments[i].hasViolation()) << "iteration: " << i;
    EXPECT_EQ(cost.violation(), assignments[i].currentViolation()) << "iteration: " << i;
    if (assignments[i].objectiveDirection() != ObjectiveDirection::NONE && assignments[i].hasObjective()) {
      EXPECT_TRUE(cost.hasObjective()) << "iteration: " << i;
      EXPECT_EQ(cost.objective(), getObj(assignments[i])) << "iteration: " << i;
    } else {
      EXPECT_FALSE(cost.hasObjective()) << "iteration: " << i;
      EXPECT_EQ(cost.objective(), Int{0}) << "iteration: " << i;
    }
    if (assignments[i].hasViolation()) {
      EXPECT_EQ(cost.satisfiesConstraints(), assignments[i].currentViolation() == 0);
    } else {
      EXPECT_TRUE(cost.satisfiesConstraints());
    }
  }
}

TEST_F(CostTest, lessThan) {
  const auto costs = generateCosts();
  for (size_t i = 0; i < costs.size(); i++) {
    for (size_t j = 0; j < costs.size(); j++) {
      if (!costs[i].hasViolation() && !costs[i].hasObjective()) {
        if (costs[j].hasViolation() || costs[j].hasObjective()) {
          EXPECT_FALSE(costs[i] < costs[j]) << "iteration: " << i;
          EXPECT_TRUE(costs[j] < costs[i]) << "iteration: " << i;
        } else {
          EXPECT_FALSE(costs[i] < costs[j]) << "iteration: " << i;
          EXPECT_FALSE(costs[j] < costs[i]) << "iteration: " << i;
        }
      } else if (!costs[j].hasViolation() && !costs[j].hasObjective()) {
        EXPECT_TRUE(costs[i] < costs[j]) << "iteration: " << i;
        EXPECT_FALSE(costs[j] < costs[i]) << "iteration: " << i;
      } else if (costs[i].hasObjective() != costs[j].hasObjective() || costs[i].hasViolation() != costs[j].hasViolation()) {
        EXPECT_FALSE(costs[i] < costs[j]) << "iteration: " << i << ", " << j;
        EXPECT_FALSE(costs[j] < costs[i]) << "iteration: " << i << ", " << j;
      } else {
        const bool violLt = costs[i].hasViolation() ? costs[i].violation() < costs[j].violation() : false;
        const bool violEq = costs[i].hasViolation() ? costs[i].violation() == costs[j].violation() : true;
        const bool objLt = costs[i].hasObjective() ? (costs[i].objective() < costs[j].objective()) : false;
        const bool objEq = costs[i].hasObjective() ? (costs[i].objective() == costs[j].objective()) : true;
        if (violEq && objEq) {
          EXPECT_FALSE(costs[i] < costs[j]) << "iteration: " << i << ", " << j;
          EXPECT_FALSE(costs[j] < costs[i]) << "iteration: " << i << ", " << j;
        } else if (violLt || (violEq && objLt)) {
          EXPECT_TRUE(costs[i] < costs[j]) << "iteration: " << i << ", " << j;
          EXPECT_FALSE(costs[j] < costs[i]) << "iteration: " << i << ", " << j;
        } else {
          EXPECT_FALSE(costs[i] < costs[j]) << "iteration: " << i << ", " << j;
          EXPECT_TRUE(costs[j] < costs[i]) << "iteration: " << i << ", " << j;
        }
      }
    }
  }
}

TEST_F(CostTest, lessEqualThen) {
  const auto costs = generateCosts();
  for (size_t i = 0; i < costs.size(); i++) {
    for (size_t j = 0; j < costs.size(); j++) {
      if (!costs[i].hasViolation() && !costs[i].hasObjective()) {
        if (costs[j].hasViolation() || costs[j].hasObjective()) {
          EXPECT_FALSE(costs[i] <= costs[j]) << "iteration: " << i;
          EXPECT_TRUE(costs[j] <= costs[i]) << "iteration: " << i;
        } else {
          EXPECT_TRUE(costs[i] <= costs[j]) << "iteration: " << i;
          EXPECT_TRUE(costs[j] <= costs[i]) << "iteration: " << i;
        }
      } else if (!costs[j].hasViolation() && !costs[j].hasObjective()) {
        EXPECT_TRUE(costs[i] <= costs[j]) << "iteration: " << i;
        EXPECT_FALSE(costs[j] <= costs[i]) << "iteration: " << i;
      } else if (costs[i].hasObjective() != costs[j].hasObjective() || costs[i].hasViolation() != costs[j].hasViolation()) {
        EXPECT_FALSE(costs[i] <= costs[j]) << "iteration: " << i << ", " << j;
        EXPECT_FALSE(costs[j] <= costs[i]) << "iteration: " << i << ", " << j;
      } else {
        const bool violLt = costs[i].hasViolation() ? costs[i].violation() < costs[j].violation() : false;
        const bool violEq = costs[i].hasViolation() ? costs[i].violation() == costs[j].violation() : true;
        const bool objLe = costs[i].hasObjective() ? (costs[i].objective() <= costs[j].objective()) : false;
        const bool objeq = costs[i].hasObjective() ? (costs[i].objective() == costs[j].objective()) : true;
        if (violEq && objeq) {
          EXPECT_TRUE(costs[i] <= costs[j]) << "iteration: " << i << ", " << j;
          EXPECT_TRUE(costs[j] <= costs[i]) << "iteration: " << i << ", " << j;
        } else if (violLt || (violEq && objLe)) {
          EXPECT_TRUE(costs[i] <= costs[j]) << "iteration: " << i << ", " << j;
          EXPECT_FALSE(costs[j] <= costs[i]) << "iteration: " << i << ", " << j;
        } else {
          EXPECT_FALSE(costs[i] <= costs[j]) << "iteration: " << i << ", " << j;
          EXPECT_TRUE(costs[j] <= costs[i]) << "iteration: " << i << ", " << j;
        }
      }
    }
  }
}

Cost genCost(const bool hasViol, const bool hasObj) {
  const Int viol = hasViol ? *rc::gen::arbitrary<Int>() : 0;
  const Int obj = hasObj ? *rc::gen::arbitrary<Int>() : 0;
  const bool isMinimize = hasObj ? *rc::gen::arbitrary<bool>() : true;
  if (hasViol && hasObj) {
    return Cost(viol, obj, isMinimize);
  }
  if (hasViol) {
    return Cost(viol);
  }
  if (hasObj) {
    return Cost(obj, isMinimize);
  }
  return Cost();
}

Int evaluate(const Cost& c, UInt violationWeight, UInt objectiveWeight) {
  RC_ASSERT(c.hasViolation());
  RC_ASSERT(c.hasObjective());
  Int violProd = 0;
  Int objProd = 0;
  Int sum = 0;
  if (!mul_overflow(static_cast<Int>(violationWeight),
                    c.violation(), violProd) &&
      !mul_overflow(c.objective(), static_cast<Int>(objectiveWeight), objProd) &&
      !add_overflow(violProd, objProd, sum)) {
    return sum;
  }
  return std::numeric_limits<Int>::max();
}

RC_GTEST_FIXTURE_PROP(CostTest, rapidcheck, ()) {
  const bool hasViol = *rc::gen::arbitrary<bool>();
  const bool hasObj = *rc::gen::arbitrary<bool>();
  const Cost c1 = genCost(hasViol, hasObj);
  const Cost c2 = genCost(hasViol, hasObj);

  const UInt vw = hasObj ? *rc::gen::arbitrary<UInt>() : UInt{0};
  const UInt ow = hasViol ? *rc::gen::arbitrary<UInt>() : UInt{0};

  const Int e1 = c1.evaluate(vw, ow);
  const Int e2 = c2.evaluate(vw, ow);

  if (!hasViol || !hasObj) {
    if (c1 < c2) {
      RC_ASSERT(e1 < e2);
    }
    if (c2 < c1) {
      RC_ASSERT(e2 < e1);
    }
    if (c1 <= c2) {
      RC_ASSERT(e1 <= e2);
    }
    if (c2 <= c1) {
      RC_ASSERT(e2 <= e1);
    }
  } else {
    const Int e1expected = evaluate(c1, vw, ow);
    EXPECT_EQ(e1expected, e1);
    const Int e2expected = evaluate(c2, vw, ow);
    EXPECT_EQ(e2expected, e2);
  }
}

}  // namespace atlantis::testing
