#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <deque>
#include <random>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::propagation;

class ViewTest : public ::testing::Test {
 protected:
  std::shared_ptr<propagation::Solver> _solver;
  std::mt19937 gen;

  VarViewId inputVar{NULL_ID};
  Int inputVarLb{0};
  Int inputVarUb{1};
  std::uniform_int_distribution<Int> inputVarDist;

  VarViewId outputVar{NULL_ID};

 public:
  void makeInputVar() {
    _solver->makeIntVar(inputVarLb, inputVarUb, inputVarDist(gen));
  }

  void SetUp() override {
    std::random_device rd;
    gen = std::mt19937(rd());
    _solver = std::make_unique<propagation::Solver>();
    inputVarDist = std::uniform_int_distribution<Int>(inputVarLb, inputVarUb);
  }
};

}  // namespace atlantis::testing
