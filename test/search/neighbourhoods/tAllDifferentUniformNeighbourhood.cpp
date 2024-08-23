#include <gtest/gtest.h>

#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AllDifferentUniformNeighbourhoodTest : public ::testing::Test {
 public:
  propagation::Solver solver;
  search::Assignment assignment{solver, propagation::NULL_ID,
                                propagation::NULL_ID,
                                propagation::ObjectiveDirection::NONE, Int{0}};
  search::RandomProvider random{123456789};

  std::vector<search::SearchVar> vars;

  void SetUp() override {
    solver.open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarId var = solver.makeIntVar(1, 1, 4);
      vars.emplace_back(var, SearchDomain(1, 4));
    }
    solver.close();
  }
};

TEST_F(AllDifferentUniformNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::AllDifferentUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), std::vector<Int>{1, 2, 3, 4});

  assignment.assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  for (const auto& var : vars) {
    EXPECT_TRUE(solver.committedValue(var.solverId()) >= 1);
    EXPECT_TRUE(solver.committedValue(var.solverId()) <= 4);
  }
}

}  // namespace atlantis::testing
