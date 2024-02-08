#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "propagation/propagation/propagationQueue.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class PropagationQueueTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(PropagationQueueTest, init) {
  PropagationQueue queue;
  queue.initVar(1, 1);
  queue.initVar(2, 2);
  EXPECT_EQ(queue.empty(), true);
  EXPECT_EQ(queue.pop(), NULL_ID);
}

TEST_F(PropagationQueueTest, isEmpty) {
  PropagationQueue queue;
  EXPECT_EQ(queue.empty(), true);
  queue.initVar(1, 1);
  queue.initVar(2, 2);
  queue.push(1);
  EXPECT_EQ(queue.empty(), false);
  queue.push(2);
  EXPECT_EQ(queue.empty(), false);
  queue.pop();
  EXPECT_EQ(queue.empty(), false);
  queue.pop();
  EXPECT_EQ(queue.empty(), true);
}

TEST_F(PropagationQueueTest, pushAndPop) {
  PropagationQueue queue;
  for (size_t i = 1; i < 100; ++i) {
    queue.initVar(i, i);
  }
  for (size_t i = 1; i < 100; ++i) {
    queue.push(i);
  }
  for (size_t i = 1; i < 100; ++i) {
    EXPECT_EQ(queue.pop(), i);
  }
}

TEST_F(PropagationQueueTest, ignoreDuplicates) {
  PropagationQueue queue;
  for (size_t i = 1; i < 100; ++i) {
    queue.initVar(i, i);
  }
  for (size_t i = 1; i < 100; ++i) {
    queue.push(i);
  }
  for (size_t i = 1; i < 100; ++i) {
    queue.push(i);
  }
  for (size_t i = 1; i < 100; ++i) {
    EXPECT_EQ(queue.pop(), i);
  }
  EXPECT_EQ(queue.empty(), true);
}

}  // namespace atlantis::testing
