#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

class AllDifferentUniformNeighbourhoodTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<search::Assignment> _assignment;
  std::shared_ptr<AllDifferentUniformNeighbourhood> _neighbourhood;
  search::RandomProvider _random{123456789};

  std::vector<search::SearchVar> vars;

  void SetUp() override {
    _solver = std::make_shared<propagation::Solver>();

    _solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 4);
      vars.emplace_back(var, SearchDomain(1, 4));
    }
    _solver->close();

    _neighbourhood = std::make_shared<
        search::neighbourhoods::AllDifferentUniformNeighbourhood>(
        std::vector<search::SearchVar>(vars), std::vector<Int>{1, 2, 3, 4});

    _assignment = std::make_shared<search::Assignment>(
        *_solver, *_neighbourhood, propagation::NULL_ID, propagation::NULL_ID,
        ObjectiveDirection::NONE, Int{0});
  }
};

TEST_F(AllDifferentUniformNeighbourhoodTest, all_values_are_initialised) {
  _assignment->initialise(_random);

  for (const auto& var : vars) {
    EXPECT_TRUE(_solver->committedValue(var.solverId()) >= 1);
    EXPECT_TRUE(_solver->committedValue(var.solverId()) <= 4);
  }
}

}  // namespace atlantis::testing
