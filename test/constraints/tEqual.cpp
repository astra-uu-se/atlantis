#include <iostream>
#include <random>
#include <vector>

#include "constraints/equal.hpp"
#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"

namespace {
class EqualTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<PropagationEngine>();
    e->open();
    x = e->makeIntVar(2, -100, 100);
    y = e->makeIntVar(2, -100, 100);
    violationId = e->makeIntVar(0, 0, 200);

    equal = e->makeConstraint<Equal>(violationId, x, y);
    e->close();
  }
  std::unique_ptr<PropagationEngine> e;
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
  EXPECT_EQ(e->getCommittedValue(violationId), 0);
  EXPECT_EQ(e->getValue(e->getTmpTimestamp(violationId), violationId), 0);
}

TEST_F(EqualTest, Recompute) {
  EXPECT_EQ(e->getValue(0, violationId), 0);
  EXPECT_EQ(e->getCommittedValue(violationId), 0);

  Timestamp newTime = 1;
  e->updateValue(newTime, x, 40);
  equal->recompute(newTime, *e);
  EXPECT_EQ(e->getCommittedValue(violationId), 0);
  EXPECT_EQ(e->getValue(newTime, violationId), 38);
}

TEST_F(EqualTest, NotifyChange) {
  EXPECT_EQ(e->getValue(0, violationId),
            0);  // initially the value of violationId is 0

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(e->getValue(time1, x), 2);
  e->updateValue(time1, x, 40);
  EXPECT_EQ(e->getCommittedValue(x), 2);
  EXPECT_EQ(e->getValue(time1, x), 40);
  equal->notifyIntChanged(time1, *e, unused);
  EXPECT_EQ(e->getValue(time1, violationId),
            38);  // incremental value of violationId is 0;

  e->updateValue(time1, y, 0);
  equal->notifyIntChanged(time1, *e, unused);
  auto tmpValue = e->getValue(
      time1, violationId);  // incremental value of violationId is 40;

  // Incremental computation gives the same result as recomputation
  equal->recompute(time1, *e);
  EXPECT_EQ(e->getValue(time1, violationId), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(e->getValue(time2, y), 2);
  e->updateValue(time2, y, 20);
  EXPECT_EQ(e->getCommittedValue(y), 2);
  EXPECT_EQ(e->getValue(time2, y), 20);
  equal->notifyIntChanged(time2, *e, unused);
  EXPECT_EQ(e->getValue(time2, violationId),
            18);  // incremental value of violationId is 0;
}

TEST_F(EqualTest, IncrementalVsRecompute) {
  EXPECT_EQ(e->getValue(0, violationId),
            0);  // initially the value of violationId is 0
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTime;
    // Check that we do not accidentally commit
    ASSERT_EQ(e->getCommittedValue(x), 2);
    ASSERT_EQ(e->getCommittedValue(y), 2);
    ASSERT_EQ(e->getCommittedValue(violationId),
              0);  // violationId is committed by register.

    // Set all variables
    e->updateValue(currentTime, x, distribution(gen));
    e->updateValue(currentTime, y, distribution(gen));

    // notify changes
    if (e->getCommittedValue(x) != e->getValue(currentTime, x)) {
      equal->notifyIntChanged(currentTime, *e, unused);
    }
    if (e->getCommittedValue(y) != e->getValue(currentTime, y)) {
      equal->notifyIntChanged(currentTime, *e, unused);
    }

    // incremental value
    auto tmp = e->getValue(currentTime, violationId);
    equal->recompute(currentTime, *e);

    ASSERT_EQ(tmp, e->getValue(currentTime, violationId));
  }
}

TEST_F(EqualTest, Commit) {
  EXPECT_EQ(e->getCommittedValue(violationId), 0);

  LocalId unused = -1;

  Timestamp currentTime = 1;

  e->updateValue(currentTime, x, 40);
  e->updateValue(currentTime, y, 2);  // This change is not notified and should
                                      // not have an impact on the commit

  equal->notifyIntChanged(currentTime, *e, unused);

  // Committing an invariant does not commit its output!
  // // Commit at wrong timestamp should have no impact
  // equal->commit(currentTime + 1, *e);
  // EXPECT_EQ(e->getCommittedValue(violationId), 0);
  // equal->commit(currentTime, *e);
  // EXPECT_EQ(e->getCommittedValue(violationId), 38);
}

}  // namespace
