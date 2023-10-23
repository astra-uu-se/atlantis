#include <gtest/gtest.h>

#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AllDifferentUniformNeighbourhoodTest : public ::testing::Test {
 public:
  propagation::Solver solver;
  search::Assignment assignment{solver, propagation::NULL_ID,
                                propagation::NULL_ID,
                                propagation::ObjectiveDirection::NONE};
  search::RandomProvider random{123456789};

  std::vector<search::SearchVariable> variables;
  std::vector<Int> domain{1, 2, 3, 4};

  void SetUp() override {
    solver.open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarId var = solver.makeIntVar(1, 1, 4);
      variables.emplace_back(var, SearchDomain(1, 4));
    }
    solver.close();
  }
};

TEST_F(AllDifferentUniformNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::AllDifferentUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVariable>(variables), domain, solver);

  assignment.assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  for (const auto& var : variables) {
    EXPECT_TRUE(solver.committedValue(var.solverId()) >= 1);
    EXPECT_TRUE(solver.committedValue(var.solverId()) <= 4);
  }
}

}  // namespace atlantis::testing