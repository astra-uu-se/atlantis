#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/binaryLinEqNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class BinaryLinEqNeighborhoodTest
    : public NeighborhoodTestBase<BinaryLinEqNeighborhood<true>> {
 public:
  Int numVars = 4;
  RandomProvider _random{123456789};

  std::vector<Int> _coeffs;
  std::vector<SearchVar> _vars;
  Int _bound = -1;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    _solver->open();
    for (Int i = 0; i < numVars; ++i) {
      _vars.emplace_back(_solver->makeIntVar(0, 0, 1),
                         std::make_shared<SearchDomain>(0, 1));
      _coeffs.emplace_back(i % 2 == 0 ? 1 : -1);
    }

    createNeighborhood(std::vector<Int>{_coeffs}, std::vector<SearchVar>{_vars},
                       _bound);
  }

  void expectHolds() const {
    Int curSum = 0;
    Int comSum = 0;
    for (size_t i = 0; i < _vars.size(); ++i) {
      EXPECT_LE(0, _vars.at(i).domain()->lowerBound());

      const Int curVal = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_GE(curVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(curVal, _vars.at(i).domain()->upperBound());
      curSum += curVal == 0 ? _coeffs.at(i) : 0;

      const Int comVal = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_GE(comVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(comVal, _vars.at(i).domain()->upperBound());
      comSum += comVal == 0 ? _coeffs.at(i) : 0;
    }

    EXPECT_EQ(curSum, _bound);
    EXPECT_EQ(comSum, _bound);
  }
};

TEST_F(BinaryLinEqNeighborhoodTest, initialise) {
  initialize();
  expectHolds();

  for (size_t m = 0; m < 100; ++m) {
    _neighborhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(BinaryLinEqNeighborhoodTest, randomMove) {
  for (size_t m = 0; m < 100; ++m) {
    initialize();
    expectHolds();

    EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 2);
    expectHolds();
  }
}

TEST_F(BinaryLinEqNeighborhoodTest, commitIf) {
  initialize();
  expectHolds();
  for (size_t m = 0; m < 100; ++m) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
