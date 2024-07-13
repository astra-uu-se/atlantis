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
  propagation::Solver solver;
  std::shared_ptr<search::Assignment> assignment;
  search::RandomProvider random{123456789};

  std::vector<Int> coeffs;
  std::vector<search::SearchVar> vars;
  Int bound = 7;

  void SetUp() override {
    solver.open();
    assignment = std::make_shared<search::Assignment>(
        solver, solver.makeIntVar(0, 0, 0), solver.makeIntVar(0, 0, 0),
        propagation::ObjectiveDirection::NONE);
    for (Int i = 0; i < numVars; ++i) {
      vars.emplace_back(solver.makeIntVar(0, -10, 10), SearchDomain(-10, 10));
      coeffs.emplace_back(i % 2 == 0 ? 1 : -1);
    }
    solver.close();
  }
};

TEST_F(IntLinEqNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::IntLinEqNeighbourhood neighbourhood(
      std::vector<Int>{coeffs}, std::vector<search::SearchVar>{vars}, bound,
      solver);
  for (size_t m = 0; m < 100; ++m) {
    assignment->assign(
        [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

    Int sum = 0;

    for (size_t i = 0; i < vars.size(); ++i) {
      EXPECT_TRUE(solver.currentValue(vars.at(i).solverId()) >= -10);
      EXPECT_TRUE(solver.currentValue(vars.at(i).solverId()) <= 10);
      sum += coeffs[i] * solver.currentValue(vars.at(i).solverId());
    }
    EXPECT_EQ(sum, bound);
  }
}

TEST_F(IntLinEqNeighbourhoodTest, randomMove) {
  search::neighbourhoods::IntLinEqNeighbourhood neighbourhood(
      std::vector<Int>{coeffs}, std::vector<search::SearchVar>{vars}, bound,
      solver);

  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  Int sum = 0;

  for (size_t i = 0; i < vars.size(); ++i) {
    EXPECT_TRUE(solver.currentValue(vars.at(i).solverId()) >= -10);
    EXPECT_TRUE(solver.currentValue(vars.at(i).solverId()) <= 10);
    sum += coeffs[i] * solver.currentValue(vars.at(i).solverId());
  }
  EXPECT_EQ(sum, bound);

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  for (size_t p = 0; p < 100; ++p) {
    EXPECT_TRUE(neighbourhood.randomMove(random, *assignment, annealer));

    sum = 0;

    for (size_t i = 0; i < vars.size(); ++i) {
      EXPECT_TRUE(solver.committedValue(vars.at(i).solverId()) >= -10);
      EXPECT_TRUE(solver.committedValue(vars.at(i).solverId()) <= 10);
      sum += coeffs[i] * solver.committedValue(vars.at(i).solverId());
    }
    EXPECT_EQ(sum, bound);
  }
}

}  // namespace atlantis::testing
