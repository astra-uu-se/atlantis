#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/intLinEqNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class IntLinEqNeighborhoodTest : public NeighborhoodTestBase<IntLinEqNeighborhood> {
 public:
  Int numVars = 4;
  RandomProvider _random{123456789};

  std::vector<Int> _coeffs;
  std::vector<SearchVar> _vars;
  Int _offset = 7;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    _solver->open();
    for (Int i = 0; i < numVars; ++i) {
      _vars.emplace_back(_solver->makeIntVar(0, -10, 10),
                         std::make_shared<SearchDomain>(-10, 10));
      _coeffs.emplace_back(i % 2 == 0 ? 1 : -1);
    }

    createNeighborhood(
        std::vector<Int>{_coeffs}, std::vector<SearchVar>{_vars}, _offset);
  }

  void expectHolds() const {
    Int curSum = 0;
    Int comSum = 0;
    for (size_t i = 0; i < _vars.size(); ++i) {
      const Int curVal = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_GE(curVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(curVal, _vars.at(i).domain()->upperBound());
      curSum += _coeffs.at(i) * curVal;

      const Int comVal = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_GE(comVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(comVal, _vars.at(i).domain()->upperBound());
      comSum += _coeffs.at(i) * comVal;
    }

    EXPECT_EQ(curSum, -_offset);
    EXPECT_EQ(comSum, -_offset);
  }
};

TEST_F(IntLinEqNeighborhoodTest, initialise) {
  initialize();
  expectHolds();

  for (size_t m = 0; m < 100; ++m) {
    _neighborhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(IntLinEqNeighborhoodTest, randomMove) {
  for (size_t m = 0; m < 100; ++m) {
    initialize();
    expectHolds();

    EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 2);
    expectHolds();
  }
}

TEST_F(IntLinEqNeighborhoodTest, commitIf) {
  initialize();
  expectHolds();
  for (size_t m = 0; m < 100; ++m) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
