#include <gtest/gtest.h>

#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class AllDifferentUniformNeighbourhoodTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<search::Assignment> _assignment;
  search::RandomProvider _d{123456789};

  std::vector<search::SearchVar> vars;

  void SetUp() override {
    _solver = std::make_shared<propagation::Solver>();
    _assignment = std::make_shared<search::Assignment>(
        *_solver, propagation::NULL_ID, propagation::NULL_ID,
        propagation::ObjectiveDirection::NONE, Int{0});

    _solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 4);
      vars.emplace_back(var, SearchDomain(1, 4));
    }
    _solver->close();
  }
};

TEST_F(AllDifferentUniformNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::AllDifferentUniformNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(vars), std::vector<Int>{1, 2, 3, 4});

  _assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(_d, modifier); });

  for (const auto& var : vars) {
    EXPECT_TRUE(_solver->committedValue(var.solverId()) >= 1);
    EXPECT_TRUE(_solver->committedValue(var.solverId()) <= 4);
  }
}

}  // namespace atlantis::testing
