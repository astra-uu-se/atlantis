#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "atlantis/propagation/propagation/propagationQueue.hpp"

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
  queue.initVar(VarId{0}, 1);
  queue.initVar(VarId{1}, 2);
  EXPECT_EQ(queue.empty(), true);
  EXPECT_EQ(queue.pop(), NULL_ID);
}

TEST_F(PropagationQueueTest, isEmpty) {
  PropagationQueue queue;
  EXPECT_EQ(queue.empty(), true);
  queue.initVar(VarId{0}, 1);
  queue.initVar(VarId{1}, 2);
  queue.push(VarId{0});
  EXPECT_EQ(queue.empty(), false);
  queue.push(VarId{1});
  EXPECT_EQ(queue.empty(), false);
  queue.pop();
  EXPECT_EQ(queue.empty(), false);
  queue.pop();
  EXPECT_EQ(queue.empty(), true);
}

TEST_F(PropagationQueueTest, pushAndPop) {
  PropagationQueue queue;
  for (VarId varId = 0; varId < 100; ++varId) {
    queue.initVar(varId, varId);
  }
  for (VarId varId = 0; varId < 100; ++varId) {
    queue.push(varId);
  }
  for (VarId varId = 0; varId < 100; ++varId) {
    EXPECT_EQ(queue.pop(), varId);
  }
}

TEST_F(PropagationQueueTest, ignoreDuplicates) {
  PropagationQueue queue;
  for (VarId varId = 0; varId < 100; ++varId) {
    queue.initVar(varId, varId);
  }
  for (VarId varId = 0; varId < 100; ++varId) {
    queue.push(varId);
  }
  for (VarId varId = 0; varId < 100; ++varId) {
    queue.push(varId);
  }
  for (VarId varId = 0; varId < 100; ++varId) {
    EXPECT_EQ(queue.pop(), varId);
  }
  EXPECT_EQ(queue.empty(), true);
}

}  // namespace atlantis::testing
