#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include <algorithm>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "constraints/allDifferent.hpp"

namespace {
class AllDifferentTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<Engine>();
    e->open();
    a = e->makeIntVar(1, -100, 100);
    b = e->makeIntVar(2, -100, 100);
    c = e->makeIntVar(2, -100, 100);
    violationId = e->makeIntVar(0, 0, 3);

    allDifferent = e->makeConstraint<AllDifferent>(violationId, std::vector<VarId>({a, b, c}));
    e->close();
  }
  std::unique_ptr<Engine> e;
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
  EXPECT_EQ(e->getCommitedValue(violationId), 1);
  EXPECT_EQ(e->getValue(e->getTmpTimestamp(violationId), violationId), 1);
}

TEST_F(AllDifferentTest, Recompute) {

  EXPECT_EQ(e->getValue(0, violationId), 1);
  EXPECT_EQ(e->getCommitedValue(violationId), 1);

  Timestamp newTime = 1;

  e->setValue(newTime, c, 3);
  allDifferent->recompute(newTime, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 1);
  EXPECT_EQ(e->getValue(newTime, violationId), 0);

  e->setValue(newTime, a, 2);
  allDifferent->recompute(newTime, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 1);
  EXPECT_EQ(e->getValue(newTime, violationId), 1);
}

TEST_F(AllDifferentTest, NotifyChange) {

  EXPECT_EQ(e->getValue(0, violationId), 1);

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(e->getValue(time1, a), 1);
  e->setValue(time1, a, 2);
  EXPECT_EQ(e->getCommitedValue(a), 1);
  EXPECT_EQ(e->getValue(time1, a), 2);
  allDifferent->notifyIntChanged(time1, *e, unused, e->getCommitedValue(a),
                           e->getValue(time1, a), 1);
  EXPECT_EQ(e->getValue(time1, violationId), 2);

  e->setValue(time1, b, 3);
  allDifferent->notifyIntChanged(time1, *e, unused, e->getCommitedValue(b),
                           e->getValue(time1, b), 1);
  auto tmpValue = e->getValue(time1, violationId);

  // Incremental computation gives the same result as recomputation
  allDifferent->recompute(time1, *e);
  EXPECT_EQ(e->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(e->getValue(time2, b), 2);
  e->setValue(time2, b, 20);
  EXPECT_EQ(e->getCommitedValue(b), 2);
  EXPECT_EQ(e->getValue(time2, b), 20);
  allDifferent->notifyIntChanged(time2, *e, unused, e->getCommitedValue(b),
                           e->getValue(time2, b), 1);
  EXPECT_EQ(e->getValue(time2, violationId), 0);
}


TEST_F(AllDifferentTest, IncrementalVsRecompute) {

  EXPECT_EQ(e->getValue(0, violationId), 1);  // initially the value of violationId is 0
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100, 100);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 2; ++i) { 
    ++currentTime;
    // Check that we do not accidentally commit
    ASSERT_EQ(e->getCommitedValue(a), 1);
    ASSERT_EQ(e->getCommitedValue(b), 2);
    ASSERT_EQ(e->getCommitedValue(violationId), 1);  // violationId is commited by register.

    // Set all variables
    e->setValue(currentTime, a, distribution(gen));
    e->setValue(currentTime, b, distribution(gen));
    
    // notify changes
    if (e->getCommitedValue(a) != e->getValue(currentTime, a)) {
      allDifferent->notifyIntChanged(currentTime, *e, unused, e->getCommitedValue(a),
                               e->getValue(currentTime, a), 1);
    }
    if (e->getCommitedValue(b) != e->getValue(currentTime, b)) {
      allDifferent->notifyIntChanged(currentTime, *e, unused, e->getCommitedValue(b),
                               e->getValue(currentTime, b), 1);
    }
    
    // incremental value
    auto tmp = e->getValue(currentTime, violationId);
    allDifferent->recompute(currentTime, *e);

    ASSERT_EQ(tmp, e->getValue(currentTime, violationId));
  }
}
/*
TEST_F(AllDifferentTest, Commit) {
  EXPECT_EQ(e->getCommitedValue(violationId), 0);

  LocalId unused = -1;

  Timestamp currentTime = 1;

  e->setValue(currentTime, a, 40);
  e->setValue(currentTime, b, 2);  // This change is not notified and should not
                                   // have an impact on the commit

  allDifferent->notifyIntChanged(currentTime, *e, unused, e->getCommitedValue(a),
                           e->getValue(currentTime, a), 1);

  // Commit at wrong timestamp should have no impact
  allDifferent->commit(currentTime + 1, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 0);
  allDifferent->commit(currentTime, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 38);
}
*/
}  // namespace