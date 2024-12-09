#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class AllDifferentUniformNeighborhoodTest : public NeighborhoodTestBase {
 public:
  std::shared_ptr<AllDifferentUniformNeighborhood> _neighborhood;
  std::vector<SearchVar> _vars;

  void expectHolds() {
    std::unordered_map<Int, bool> holdsCurVal;
    std::unordered_map<Int, bool> holdsComVal;
    holdsCurVal.reserve(_vars.size());
    holdsComVal.reserve(_vars.size());
    if (_vars.empty()) {
      return;
    }
    if (_vars.front().constDomain().isInterval()) {
      for (Int val = _vars.front().constDomain().lowerBound();
           val <= _vars.front().constDomain().upperBound(); ++val) {
        holdsCurVal.emplace(val, false);
        holdsComVal.emplace(val, false);
      }
    } else {
      for (const Int val : _vars.front().domain().values()) {
        holdsCurVal.emplace(val, false);
        holdsComVal.emplace(val, false);
      }
    }
    for (const auto& var : _vars) {
      const Int curVal = _solver->currentValue(var.solverId());
      const Int comVal = _solver->committedValue(var.solverId());
      EXPECT_TRUE(var.constDomain().contains(curVal));
      EXPECT_TRUE(var.constDomain().contains(comVal));

      EXPECT_TRUE(holdsCurVal.contains(curVal));
      EXPECT_TRUE(holdsComVal.contains(comVal));

      EXPECT_FALSE(holdsCurVal.at(curVal));
      EXPECT_FALSE(holdsComVal.at(comVal));

      holdsCurVal.at(curVal) = true;
      holdsComVal.at(comVal) = true;
    }
  }

  void SetUp() override {
    NeighborhoodTestBase::SetUp();
    _solver->open();

    for (size_t i = 0; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 5);
      _vars.emplace_back(var, SearchDomain(1, 5));
    }
    _solver->close();

    _neighborhood =
        std::make_shared<neighborhoods::AllDifferentUniformNeighborhood>(
            std::vector<SearchVar>(_vars), std::vector<Int>{1, 2, 3, 4, 5});
  }
};

TEST_F(AllDifferentUniformNeighborhoodTest, initialize) {
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    initialize(*_neighborhood);
    expectHolds();
  }
}

TEST_F(AllDifferentUniformNeighborhoodTest, swap) {
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    initialize(*_neighborhood);
    expectHolds();
    EXPECT_EQ(_neighborhood->swapValues(_random, *_assignment), 2);
    std::vector<propagation::VarId> modified;
    modified.reserve(2);
    for (const auto& var : _vars) {
      if (_solver->hasChanged(_solver->currentTimestamp(), var.solverId())) {
        modified.emplace_back(var.solverId());
      }
    }
    EXPECT_EQ(modified.size(), 2);
    EXPECT_EQ(_solver->committedValue(modified.front()),
              _solver->currentValue(modified.back()));
    EXPECT_EQ(_solver->committedValue(modified.back()),
              _solver->currentValue(modified.front()));

    expectHolds();
  }
}

TEST_F(AllDifferentUniformNeighborhoodTest, assignValue) {
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    initialize(*_neighborhood);
    expectHolds();
    EXPECT_EQ(_neighborhood->assignValue(_random, *_assignment), 1);
    propagation::VarId modified{propagation::NULL_ID};
    for (const auto& var : _vars) {
      if (_solver->hasChanged(_solver->currentTimestamp(), var.solverId())) {
        EXPECT_EQ(modified, propagation::NULL_ID);
        modified = var.solverId();
      }
    }
    EXPECT_NE(modified, propagation::NULL_ID);
    EXPECT_NE(_solver->committedValue(modified),
              _solver->currentValue(modified));
    expectHolds();
  }
}

TEST_F(AllDifferentUniformNeighborhoodTest, randomMove) {
  initialize(*_neighborhood);
  expectHolds();
  for (size_t iteration = 0; iteration < 1000; ++iteration) {
    commitIf(*_neighborhood);
    expectHolds();
  }
}

}  // namespace atlantis::testing
