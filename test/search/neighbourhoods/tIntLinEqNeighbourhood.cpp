#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighbourhoods/intLinEqNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

class IntLinEqNeighbourhoodTest : public ::testing::Test {
 public:
  Int numVars = 4;
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<Assignment> _assignment;
  std::shared_ptr<IntLinEqNeighbourhood> _neighbourhood;
  RandomProvider _random{123456789};

  std::vector<Int> coeffs;
  std::vector<SearchVar> vars;
  Int offset = 7;

  void SetUp() override {
    _solver = std::make_shared<propagation::Solver>();
    _solver->open();
    for (Int i = 0; i < numVars; ++i) {
      vars.emplace_back(_solver->makeIntVar(0, -10, 10), SearchDomain(-10, 10));
      coeffs.emplace_back(i % 2 == 0 ? 1 : -1);
    }

    _neighbourhood = std::make_shared<neighbourhoods::IntLinEqNeighbourhood>(
        std::vector<Int>{coeffs}, std::vector<SearchVar>{vars}, offset);

    _assignment = std::make_shared<Assignment>(
        *_solver, *_neighbourhood, _solver->makeIntVar(0, 0, 0),
        _solver->makeIntVar(0, 0, 0), ObjectiveDirection::NONE, Int{0});
    _solver->close();
  }
};

TEST_F(IntLinEqNeighbourhoodTest, all_values_are_initialised) {
  for (size_t m = 0; m < 100; ++m) {
    _assignment->initialise(_random);

    Int sum = 0;

    for (size_t i = 0; i < vars.size(); ++i) {
      EXPECT_TRUE(_solver->currentValue(vars.at(i).solverId()) >= -10);
      EXPECT_TRUE(_solver->currentValue(vars.at(i).solverId()) <= 10);
      sum += coeffs[i] * _solver->currentValue(vars.at(i).solverId());
    }
    EXPECT_EQ(sum, -offset);
  }
}

TEST_F(IntLinEqNeighbourhoodTest, randomMove) {
  _assignment->initialise(_random);

  Int sum = 0;

  for (size_t i = 0; i < vars.size(); ++i) {
    EXPECT_TRUE(_solver->currentValue(vars.at(i).solverId()) >= -10);
    EXPECT_TRUE(_solver->currentValue(vars.at(i).solverId()) <= 10);
    sum += coeffs[i] * _solver->currentValue(vars.at(i).solverId());
  }
  EXPECT_EQ(sum, -offset);

  for (size_t p = 0; p < 100; ++p) {
    EXPECT_GT(_neighbourhood->randomMove(_random, *_assignment), size_t{0});

    sum = 0;

    for (size_t i = 0; i < vars.size(); ++i) {
      EXPECT_TRUE(_solver->committedValue(vars.at(i).solverId()) >= -10);
      EXPECT_TRUE(_solver->committedValue(vars.at(i).solverId()) <= 10);
      sum += coeffs[i] * _solver->committedValue(vars.at(i).solverId());
    }
    EXPECT_EQ(sum, -offset);
  }
}

}  // namespace atlantis::testing
