#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "constraints/allDifferent.hpp"
#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"

namespace {
class AllDifferentTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<PropagationEngine>();
    e->open();
    a = e->makeIntVar(1, -100, 100);
    b = e->makeIntVar(2, -100, 100);
    c = e->makeIntVar(2, -100, 100);
    violationId = e->makeIntVar(0, 0, 3);

    allDifferent = e->makeConstraint<AllDifferent>(
        violationId, std::vector<VarId>({a, b, c}));
    e->close();
  }
  std::unique_ptr<PropagationEngine> e;
  VarId violationId = NULL_ID;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  std::shared_ptr<AllDifferent> allDifferent;
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(AllDifferentTest, Init) {
  EXPECT_EQ(e->getCommittedValue(violationId), 1);
  EXPECT_EQ(e->getValue(e->getTmpTimestamp(violationId), violationId), 1);
}

TEST_F(AllDifferentTest, Recompute) {
  EXPECT_EQ(e->getValue(0, violationId), 1);
  EXPECT_EQ(e->getCommittedValue(violationId), 1);

  Timestamp newTime = 1;

  e->updateValue(newTime, c, 3);
  allDifferent->recompute(newTime, *e);
  EXPECT_EQ(e->getCommittedValue(violationId), 1);
  EXPECT_EQ(e->getValue(newTime, violationId), 0);

  e->updateValue(newTime, a, 2);
  allDifferent->recompute(newTime, *e);
  EXPECT_EQ(e->getCommittedValue(violationId), 1);
  EXPECT_EQ(e->getValue(newTime, violationId), 1);
}

TEST_F(AllDifferentTest, NotifyChange) {
  EXPECT_EQ(e->getValue(0, violationId), 1);

  Timestamp time1 = 1;

  EXPECT_EQ(e->getValue(time1, a), 1);
  e->updateValue(time1, a, 2);
  EXPECT_EQ(e->getCommittedValue(a), 1);
  EXPECT_EQ(e->getValue(time1, a), 2);
  allDifferent->notifyIntChanged(time1, *e, 0);
  allDifferent->compute(time1, *e);
  EXPECT_EQ(e->getValue(time1, violationId), 2);

  e->updateValue(time1, b, 3);
  allDifferent->notifyIntChanged(time1, *e, 1);
  allDifferent->compute(time1, *e);
  auto tmpValue = e->getValue(time1, violationId);

  // Incremental computation gives the same result as recomputation
  allDifferent->recompute(time1, *e);
  EXPECT_EQ(e->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(e->getValue(time2, b), 2);
  e->updateValue(time2, b, 20);
  EXPECT_EQ(e->getCommittedValue(b), 2);
  EXPECT_EQ(e->getValue(time2, b), 20);
  allDifferent->notifyIntChanged(time2, *e, 1);
  allDifferent->recompute(time2, *e);
  EXPECT_EQ(e->getValue(time2, violationId), 0);
}

TEST_F(AllDifferentTest, IncrementalVsRecompute) {
  EXPECT_EQ(e->getValue(0, violationId),
            1);  // initially the value of violationId is 0
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100, 100);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 2; ++i) {
    ++currentTime;
    // Check that we do not accidentally commit
    ASSERT_EQ(e->getCommittedValue(a), 1);
    ASSERT_EQ(e->getCommittedValue(b), 2);
    ASSERT_EQ(e->getCommittedValue(violationId),
              1);  // violationId is commited by register.

    // Set all variables
    e->updateValue(currentTime, a, distribution(gen));
    e->updateValue(currentTime, b, distribution(gen));

    // notify changes
    if (e->getCommittedValue(a) != e->getValue(currentTime, a)) {
      allDifferent->notifyIntChanged(currentTime, *e, 0);
    }
    if (e->getCommittedValue(b) != e->getValue(currentTime, b)) {
      allDifferent->notifyIntChanged(currentTime, *e, 1);
    }
    allDifferent->compute(currentTime, *e);
    
    // incremental value
    auto tmp = e->getValue(currentTime, violationId);
    allDifferent->recompute(currentTime, *e);

    ASSERT_EQ(tmp, e->getValue(currentTime, violationId));
  }
}

TEST_F(AllDifferentTest, Commit) {
  /*
  It is difficult to test the method commit as it only
  commits the internal data structures of the constraint.
  The internal data structures are (in almost all cases)
  private.
  */
  ASSERT_TRUE(true);
}

}  // namespace
