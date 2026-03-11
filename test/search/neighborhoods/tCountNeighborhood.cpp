#include <gtest/gtest.h>

#include "./testHelper.hpp"
#include "atlantis/search/neighborhoods/countNeighborhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighborhoods;

class CountNeighborhoodTest : public NeighborhoodTestBase<CountNeighborhood> {
 public:
  Int numVars = 4;
  RandomProvider _random{123456789};
  std::vector<SearchVar> _vars;
  Int needle = 1;
  size_t amount = 2;

  void SetUp() override {
    NeighborhoodTestBase::SetUp();

    _solver->open();
    for (Int i = 0; i < numVars; ++i) {
      _vars.emplace_back(_solver->makeIntVar(0, -10, 10),
                         std::make_shared<SearchDomain>(-10, 10));
    }

    createNeighborhood(std::vector<SearchVar>{_vars}, needle, amount);
  }

  void expectHolds() const {
    size_t curAmount = 0;
    size_t comAmount = 0;
    for (size_t i = 0; i < _vars.size(); ++i) {
      const Int curVal = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_GE(curVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(curVal, _vars.at(i).domain()->upperBound());
      curAmount += curVal == needle ? 1 : 0;

      const Int comVal = _solver->committedValue(_vars.at(i).solverId());
      EXPECT_GE(comVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(comVal, _vars.at(i).domain()->upperBound());
      comAmount += comVal == needle ? 1 : 0;
    }

    EXPECT_EQ(curAmount, amount);
    EXPECT_EQ(comAmount, amount);
  }
};

TEST_F(CountNeighborhoodTest, initialise) {
  initialize();
  expectHolds();

  for (size_t m = 0; m < 100; ++m) {
    _neighborhood->initialize(_random, *_assignment);
    expectHolds();
  }
}

TEST_F(CountNeighborhoodTest, randomMove) {
  for (size_t m = 0; m < 100; ++m) {
    initialize();
    expectHolds();

    EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 2);
    expectHolds();
  }
}

TEST_F(CountNeighborhoodTest, commitIf) {
  initialize();
  expectHolds();
  for (size_t m = 0; m < 100; ++m) {
    commitIf();
    expectHolds();
  }
}

}  // namespace atlantis::testing
