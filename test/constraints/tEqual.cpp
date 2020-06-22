#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "constraints/equal.hpp"

namespace {
class EqualTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<Engine>();
    e->open();
    violationId = e->makeIntVar(0);
    x = e->makeIntVar(2);
    y = e->makeIntVar(2);

    // violationId = 1*1+2*10+3*(-20) = 1+20-60 =0
    equal = e->makeConstraint<Equal>(violationId, x, y);
    e->close();
  }
  std::unique_ptr<Engine> e;
  VarId violationId = NULL_ID;
  VarId x = NULL_ID;
  VarId y = NULL_ID;
  std::shared_ptr<Equal> equal;
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(EqualTest, Init) {
  EXPECT_EQ(e->getCommitedValue(violationId), 0);
  EXPECT_EQ(e->getValue(e->getTmpTimestamp(violationId), violationId), 0);
}

TEST_F(EqualTest, Recompute) {

  EXPECT_EQ(e->getValue(0, violationId), 0);
  EXPECT_EQ(e->getCommitedValue(violationId), 0);

  Timestamp newTime = 1;
  e->setValue(newTime, x, 40);
  equal->recompute(newTime, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 0);
  EXPECT_EQ(e->getValue(newTime, violationId), 38);
}

TEST_F(EqualTest, NotifyChange) {

  EXPECT_EQ(e->getValue(0, violationId), 0);  // initially the value of violationId is 0

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(e->getValue(time1, x), 2);
  e->setValue(time1, x, 40);
  EXPECT_EQ(e->getCommitedValue(x), 2);
  EXPECT_EQ(e->getValue(time1, x), 40);
  equal->notifyIntChanged(time1, *e, unused, e->getCommitedValue(x),
                           e->getValue(time1, x), 1);
  EXPECT_EQ(e->getValue(time1, violationId), 38);  // incremental value of violationId is 0;

  e->setValue(time1, y, 0);
  equal->notifyIntChanged(time1, *e, unused, e->getCommitedValue(y),
                           e->getValue(time1, y), 1);
  auto tmpValue = e->getValue(time1, violationId);  // incremental value of violationId is 40;

  // Incremental computation gives the same result as recomputation
  equal->recompute(time1, *e);
  EXPECT_EQ(e->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(e->getValue(time2, y), 2);
  e->setValue(time2, y, 20);
  EXPECT_EQ(e->getCommitedValue(y), 2);
  EXPECT_EQ(e->getValue(time2, y), 20);
  equal->notifyIntChanged(time2, *e, unused, e->getCommitedValue(y),
                           e->getValue(time2, y), 1);
  EXPECT_EQ(e->getValue(time2, violationId), 18);  // incremental value of violationId is 0;
}

TEST_F(EqualTest, IncrementalVsRecompute) {

  EXPECT_EQ(e->getValue(0, violationId), 0);  // initially the value of violationId is 0
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 1000; ++i) { 
    ++currentTime;
    // Check that we do not accidentally commit
    EXPECT_EQ(e->getCommitedValue(x), 2);
    EXPECT_EQ(e->getCommitedValue(y), 2);
    EXPECT_EQ(e->getCommitedValue(violationId), 0);  // violationId is commited by register.

    // Set all variables
    e->setValue(currentTime, x, distribution(gen));
    e->setValue(currentTime, y, distribution(gen));
    
    // notify changes
    if (e->getCommitedValue(x) != e->getValue(currentTime, x)) {
      equal->notifyIntChanged(currentTime, *e, unused, e->getCommitedValue(x),
                               e->getValue(currentTime, x), 1);
    }
    if (e->getCommitedValue(y) != e->getValue(currentTime, y)) {
      equal->notifyIntChanged(currentTime, *e, unused, e->getCommitedValue(y),
                               e->getValue(currentTime, y), 1);
    }
    
    // incremental value
    auto tmp = e->getValue(currentTime, violationId);
    equal->recompute(currentTime, *e);

    EXPECT_EQ(tmp, e->getValue(currentTime, violationId));
  }
}

TEST_F(EqualTest, Commit) {
  EXPECT_EQ(e->getCommitedValue(violationId), 0);

  LocalId unused = -1;

  Timestamp currentTime = 1;

  e->setValue(currentTime, x, 40);
  e->setValue(currentTime, y, 2);  // This change is not notified and should not
                                   // have an impact on the commit

  equal->notifyIntChanged(currentTime, *e, unused, e->getCommitedValue(x),
                           e->getValue(currentTime, x), 1);

  // Commit at wrong timestamp should have no impact
  equal->commit(currentTime + 1, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 0);
  equal->commit(currentTime, *e);
  EXPECT_EQ(e->getCommitedValue(violationId), 38);
}

}  // namespace