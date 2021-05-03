#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

#include "core/types.hpp"
#include "gtest/gtest.h"
#include "misc/logging.hpp"
#include "rapidcheck/gtest.h"
#include "utils/priorityList.hpp"

namespace {
class PriorityListTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;

  void updateForward(Timestamp t, PriorityList &p) { updateForward(t, p, 0); }
  void updateForward(Timestamp t, PriorityList &p, Int offset) {
    for (size_t idx = 0; idx < p.size(); ++idx) {
      p.updatePriority(t, idx, idx + 1 + offset);
      p.sanity(t);
    }
  }

  void updateBackwards(Timestamp t, PriorityList &p) {
    updateBackwards(t, p, 0);
  }
  void updateBackwards(Timestamp t, PriorityList &p, Int offset) {
    for (size_t idx = 0; idx < p.size(); ++idx) {
      p.updatePriority(t, idx, p.size() - idx + offset);
      p.sanity(t);
    }
  }

  void updateUniform(Timestamp t, PriorityList &p) {
    for (size_t idx = 0; idx < p.size(); ++idx) {
      p.updatePriority(t, idx, Int(t));
      p.sanity(t);
    }
  }
};

/**
 *  Testing constructor
 */

TEST_F(PriorityListTest, Constructor) {
  for (size_t size = 0; size < 100; ++size) {
    PriorityList p(size);
    EXPECT_EQ(p.size(), size);
    p.sanity(0);
  }
}

TEST_F(PriorityListTest, SimpleUpdatePriority) {
  size_t size = 100;
  Timestamp t;
  PriorityList p(size);

  for (t = 1; t < 10; ++t) {
    updateForward(t, p);
    EXPECT_EQ(p.getMinPriority(t), 1);
    EXPECT_EQ(p.getMaxPriority(t), 100);
  }
  for (t = 1; t < 10; ++t) {
    updateBackwards(t, p);
    EXPECT_EQ(p.getMinPriority(t), 1);
    EXPECT_EQ(p.getMaxPriority(t), 100);
  }
  for (t = 1; t < 10; ++t) {
    updateUniform(t, p);
    EXPECT_EQ(p.getMinPriority(t), Int(t));
    EXPECT_EQ(p.getMaxPriority(t), Int(t));
  }

  for (t = 1; t < 10; ++t) {
    updateForward(t, p);
    EXPECT_EQ(p.getMinPriority(t), 1);
    EXPECT_EQ(p.getMaxPriority(t), 100);

    updateBackwards(t, p);
    EXPECT_EQ(p.getMinPriority(t), 1);
    EXPECT_EQ(p.getMaxPriority(t), 100);

    updateUniform(t, p);
    EXPECT_EQ(p.getMinPriority(t), Int(t));
    EXPECT_EQ(p.getMaxPriority(t), Int(t));
  }
}

TEST_F(PriorityListTest, RandomUpdatePriority) {
  setLogLevel(debug);
  for (size_t n = 0; n < 1000; ++n) {
    size_t size = 100;
    Timestamp t = 1;
    std::uniform_int_distribution<> distribution(
        std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    PriorityList p(size);
    Int minPriority = std::numeric_limits<int>::max();
    Int maxPriority = std::numeric_limits<int>::min();
    for (size_t idx = 0; idx < 100; ++idx) {
      Int newValue = distribution(gen);
      minPriority = std::min(newValue, minPriority);
      maxPriority = std::max(newValue, maxPriority);
      p.updatePriority(t, idx, newValue);
      p.sanity(t);
    }

    EXPECT_EQ(p.getMinPriority(t), minPriority);
    EXPECT_EQ(p.getMaxPriority(t), maxPriority);
  }
}

TEST_F(PriorityListTest, CommitIf) {
  size_t size = 100;
  PriorityList p(size);
  Timestamp t = 1;

  updateForward(t, p, Int(t));
  p.commitIf(t);
  EXPECT_EQ(p.getMinPriority(t), 1 + Int(t));
  EXPECT_EQ(p.getMaxPriority(t), 100 + Int(t));
  EXPECT_EQ(p.getMinPriority(t + 1), 1 + Int(t));
  EXPECT_EQ(p.getMaxPriority(t + 1), 100 + Int(t));

  for (t = 2; t < 10; ++t) {
    updateForward(t, p, Int(t));
    EXPECT_EQ(p.getMinPriority(t), 1 + Int(t));
    EXPECT_EQ(p.getMaxPriority(t), 100 + Int(t));
    p.commitIf(t);
    EXPECT_EQ(p.getMinPriority(t + 1), 1 + Int(t));
    EXPECT_EQ(p.getMaxPriority(t + 1), 100 + Int(t));
  }
}

}  // namespace