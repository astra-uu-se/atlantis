#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/neighbourhoods/intLinEqNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class IntLinEqNeighbourhoodTest : public ::testing::Test {
 public:
  Int numVars = 4;
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<search::Assignment> _assignment;
  search::RandomProvider _random{123456789};

  std::vector<Int> coeffs;
  std::vector<search::SearchVar> vars;
  Int offset = 7;

  void SetUp() override {
    _solver = std::make_shared<propagation::Solver>();
    _solver->open();
    _assignment = std::make_shared<search::Assignment>(
        *_solver, _solver->makeIntVar(0, 0, 0), _solver->makeIntVar(0, 0, 0),
        propagation::ObjectiveDirection::NONE, Int{0});
    for (Int i = 0; i < numVars; ++i) {
      vars.emplace_back(_solver->makeIntVar(0, -10, 10), SearchDomain(-10, 10));
      coeffs.emplace_back(i % 2 == 0 ? 1 : -1);
    }
    _solver->close();
  }
};

TEST_F(IntLinEqNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::IntLinEqNeighbourhood neighbourhood(
      std::vector<Int>{coeffs}, std::vector<search::SearchVar>{vars}, offset);
  for (size_t m = 0; m < 100; ++m) {
    _assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

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
  search::neighbourhoods::IntLinEqNeighbourhood neighbourhood(
      std::vector<Int>{coeffs}, std::vector<search::SearchVar>{vars}, offset);

  _assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

  Int sum = 0;

  for (size_t i = 0; i < vars.size(); ++i) {
    EXPECT_TRUE(_solver->currentValue(vars.at(i).solverId()) >= -10);
    EXPECT_TRUE(_solver->currentValue(vars.at(i).solverId()) <= 10);
    sum += coeffs[i] * _solver->currentValue(vars.at(i).solverId());
  }
  EXPECT_EQ(sum, -offset);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*_assignment, _random, *schedule);

  for (size_t p = 0; p < 100; ++p) {
    EXPECT_TRUE(neighbourhood.randomMove(_random, *_assignment, annealer));

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
