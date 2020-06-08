#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/engine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/linear.hpp"

// The fixture for testing class IntDomain. From google test primer.
namespace {
class LinearTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<Engine>();
    a = e->makeIntVar();
    b = e->makeIntVar();
    c = e->makeIntVar();
    d = e->makeIntVar();
    a->commitValue(1);
    b->commitValue(2);
    c->commitValue(3);
    d->commitValue(4);

    // d = 1*1+2*10+3*(-20) = 1+20-60 =-39
    linear = std::make_shared<Linear>(
        std::vector<Int>({1, 10, -20}),
        std::vector<std::shared_ptr<IntVar>>({a, b, c}), d);
  }
  std::unique_ptr<Engine> e;
  std::shared_ptr<IntVar> a;
  std::shared_ptr<IntVar> b;
  std::shared_ptr<IntVar> c;
  std::shared_ptr<IntVar> d;
  std::shared_ptr<Linear> linear;
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(LinearTest, Init) {
  e->registerInvariant(linear);
  EXPECT_EQ(d->getCommittedValue(), -39);
  EXPECT_EQ(d->getValue(d->getTimestamp()), -39);
}

TEST_F(LinearTest, Recompute) {
  e->registerInvariant(linear);

  EXPECT_EQ(d->getValue(0), -39);
  EXPECT_EQ(d->getCommittedValue(), -39);

  Timestamp newTime = 1;
  e->setValue(newTime,*a, 40);
  linear->recompute(newTime, *e);
  EXPECT_EQ(d->getCommittedValue(), -39);
  EXPECT_EQ(d->getValue(newTime), 0);
}

TEST_F(LinearTest, NotifyChange) {
  e->registerInvariant(linear);

  EXPECT_EQ(d->getValue(0), -39);  // initially the value of d is -39

  LocalId unused = -1;

  Timestamp time1 = 1;

  EXPECT_EQ(a->getValue(time1), 1);
  e->setValue(time1,*a, 40);
  EXPECT_EQ(a->getCommittedValue(), 1);
  EXPECT_EQ(a->getValue(time1), 40);
  linear->notifyIntChanged(time1, *e, unused, a->getCommittedValue(),
                           a->getValue(time1), 1);
  EXPECT_EQ(d->getValue(time1), 0);  // incremental value of d is 0;

  e->setValue(time1,*b, 0);
  linear->notifyIntChanged(time1, *e, unused, b->getCommittedValue(),
                           b->getValue(time1), 10);
  auto tmpValue = d->getValue(time1);  // incremental value of d is -40;

  // Incremental computation gives the same result as recomputation
  linear->recompute(time1, *e);
  EXPECT_EQ(d->getValue(time1), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(a->getValue(time2), 1);
  e->setValue(time2,*a, 20);
  EXPECT_EQ(a->getCommittedValue(), 1);
  EXPECT_EQ(a->getValue(time2), 20);
  linear->notifyIntChanged(time2, *e, unused, a->getCommittedValue(),
                           a->getValue(time2), 1);
  EXPECT_EQ(d->getValue(time2), -20);  // incremental value of d is 0;
}

TEST_F(LinearTest, IncrementalVsRecompute) {
  e->registerInvariant(linear);

  EXPECT_EQ(d->getValue(0), -39);  // initially the value of d is -39
  LocalId unused = -1;
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 1000; i++) {
    currentTime++;
    // Check that we do not accidentally commit
    EXPECT_EQ(a->getCommittedValue(), 1);
    EXPECT_EQ(b->getCommittedValue(), 2);
    EXPECT_EQ(c->getCommittedValue(), 3);
    EXPECT_EQ(d->getCommittedValue(), -39);  // d is commited by register.

    // Set all variables
    e->setValue(currentTime,*a, distribution(gen));
    e->setValue(currentTime,*b, distribution(gen));
    e->setValue(currentTime,*c, distribution(gen));

    // notify changes
    if (a->getCommittedValue() != a->getValue(currentTime)) {
      linear->notifyIntChanged(currentTime, *e, unused, a->getCommittedValue(),
                               a->getValue(currentTime), 1);
    }
    if (b->getCommittedValue() != b->getValue(currentTime)) {
      linear->notifyIntChanged(currentTime, *e, unused, b->getCommittedValue(),
                               b->getValue(currentTime), 10);
    }
    if (c->getCommittedValue() != c->getValue(currentTime)) {
      linear->notifyIntChanged(currentTime, *e, unused, c->getCommittedValue(),
                               c->getValue(currentTime), -20);
    }

    // incremental value
    auto tmp = d->getValue(currentTime);
    linear->recompute(currentTime, *e);

    EXPECT_EQ(tmp, d->getValue(currentTime));
  }
}

TEST_F(LinearTest, Commit) {
  e->registerInvariant(linear);
  EXPECT_EQ(d->getCommittedValue(), -39);

  LocalId unused = -1;

  Timestamp currentTime = 1;

  e->setValue(currentTime,*a, 40);
  e->setValue(currentTime,*b, 2);  // This change is not notified and should not
                                // have an impact on the commit

  linear->notifyIntChanged(currentTime, *e, unused, a->getCommittedValue(),
                           a->getValue(currentTime), 1);

  // Commit at wrong timestamp should have no impact
  linear->commit(currentTime + 1);
  EXPECT_EQ(d->getCommittedValue(), -39);
  linear->commit(currentTime);
  EXPECT_EQ(d->getCommittedValue(), 0);
}
}  // namespace