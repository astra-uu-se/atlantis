#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/intLinLeNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class IntLinLeNeighborhoodTest
    : public NeighborhoodTestBase<IntLinLeNeighborhood> {
 public:
  Int numVars = 4;
  RandomProvider _random{123456789};

  std::vector<Int> _coeffs;
  std::vector<SearchVar> _vars;
  Int _bound = 7;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    _solver->open();
    for (Int i = 0; i < numVars; ++i) {
      _vars.emplace_back(_solver->makeIntVar(0, -10, 10),
                         std::make_shared<SearchDomain>(-10, 10));
      _coeffs.emplace_back((i % 2 == 0 ? 2 : -2) * (i + 1));
    }

    createNeighborhood(std::vector<Int>{_coeffs}, std::vector<SearchVar>{_vars},
                       _bound);
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

    EXPECT_LE(curSum, _bound);
    EXPECT_LE(comSum, _bound);
  }
};

TEST_F(IntLinLeNeighborhoodTest, initialise) {
  initialize();
  expectHolds();

  for (size_t m = 0; m < 100; ++m) {
    _neighborhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(IntLinLeNeighborhoodTest, randomMove) {
  for (size_t m = 0; m < 100; ++m) {
    initialize();
    expectHolds();

    EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 1);
    expectHolds();
  }
}

TEST_F(IntLinLeNeighborhoodTest, commitIf) {
  initialize();
  expectHolds();
  for (size_t m = 0; m < 100; ++m) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
