#include <gtest/gtest.h>

#include <algorithm>

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
      const Int curVal = _assignment->currentValue(_vars.at(i).solverId());
      EXPECT_GE(curVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(curVal, _vars.at(i).domain()->upperBound());
      curAmount += curVal == needle ? 1 : 0;

      const Int comVal = _assignment->committedValue(_vars.at(i).solverId());
      EXPECT_GE(comVal, _vars.at(i).domain()->lowerBound());
      EXPECT_LE(comVal, _vars.at(i).domain()->upperBound());
      comAmount += comVal == needle ? 1 : 0;
    }

    EXPECT_EQ(curAmount, amount);
    EXPECT_EQ(comAmount, amount);
  }

  [[nodiscard]] bool currentDiffersFromCommitted() const {
    return std::ranges::any_of(_vars, [&](const auto& var) {
      return _assignment->currentValue(var.solverId()) !=
             _assignment->committedValue(var.solverId());
    });
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
    EXPECT_TRUE(currentDiffersFromCommitted());
  }
}

TEST_F(CountNeighborhoodTest, commitIf) {
  initialize();
  expectHolds();
  for (size_t m = 0; m < 100; ++m) {
    EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 2);
    expectHolds();
    EXPECT_TRUE(currentDiffersFromCommitted());
    commitIf();
    expectHolds();
    EXPECT_FALSE(currentDiffersFromCommitted());
  }
}

TEST_F(CountNeighborhoodTest, SupportsVarsThatCannotTakeNeedle) {
  _vars.clear();
  if (_solver->isOpen()) {
    _solver->close();
  }
  _neighborhood.reset();
  _assignment.reset();
  _solver->open();
  _vars.emplace_back(_solver->makeIntVar(0, 0, 2),
                     std::make_shared<SearchDomain>(0, 2));
  _vars.emplace_back(_solver->makeIntVar(0, 0, 2),
                     std::make_shared<SearchDomain>(0, 2));
  _vars.emplace_back(_solver->makeIntVar(2, 2, 4),
                     std::make_shared<SearchDomain>(2, 4));
  _vars.emplace_back(_solver->makeIntVar(0, 0, 2),
                     std::make_shared<SearchDomain>(0, 2));
  createNeighborhood(std::vector<SearchVar>{_vars}, needle, amount);

  for (size_t m = 0; m < 25; ++m) {
    initialize();
    expectHolds();

    EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 2);
    expectHolds();
    EXPECT_TRUE(currentDiffersFromCommitted());
  }
}

TEST_F(CountNeighborhoodTest,
       RandomMoveReturnsZeroWhenAllEligibleVarsMustEqualNeedle) {
  _vars.clear();
  if (_solver->isOpen()) {
    _solver->close();
  }
  _neighborhood.reset();
  _assignment.reset();
  _solver->open();
  _vars.emplace_back(_solver->makeIntVar(0, 0, 1),
                     std::make_shared<SearchDomain>(0, 1));
  _vars.emplace_back(_solver->makeIntVar(0, 0, 1),
                     std::make_shared<SearchDomain>(0, 1));
  _vars.emplace_back(_solver->makeIntVar(2, 2, 3),
                     std::make_shared<SearchDomain>(2, 3));
  createNeighborhood(std::vector<SearchVar>{_vars}, needle, 2);

  initialize();
  expectHolds();
  EXPECT_EQ(_neighborhood->randomMove(_random, *_assignment), 0);
  expectHolds();
}

}  // namespace atlantis::testing
