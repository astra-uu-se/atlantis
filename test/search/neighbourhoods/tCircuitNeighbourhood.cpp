#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/assignment.hpp"
#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search::neighbourhoods;

class CircuitNeighbourhoodTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<CircuitNeighbourhood> _neighbourhood;
  std::shared_ptr<Assignment> _assignment;
  RandomProvider _random{123456789};
  Int _offset = 1;

  std::vector<SearchVar> next;

  void SetUp() override {
    _solver = std::make_unique<propagation::Solver>();

    _solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarViewId var = _solver->makeIntVar(1, 1, 4);
      next.emplace_back(var, SearchDomain(1, 4));
    }

    propagation::VarViewId objective = _solver->makeIntVar(0, 0, 0);
    propagation::VarViewId violation = _solver->makeIntVar(0, 0, 0);
    _solver->close();

    _neighbourhood = std::make_shared<CircuitNeighbourhood>(
        std::vector<SearchVar>(next), _offset);

    _assignment =
        std::make_shared<Assignment>(*_solver, *_neighbourhood, objective,
                                     violation, ObjectiveDirection::NONE, 0);
  }

  void expectCycle() {
    std::vector<bool> visited(next.size(), false);
    Int cur = 0;
    while (!visited.at(cur)) {
      visited.at(cur) = true;
      cur = _solver->committedValue(next.at(cur).solverId()) - _offset;
      EXPECT_GE(cur, 0);
      EXPECT_LT(cur, next.size());
    }

    for (size_t i = 0; i < next.size(); ++i) {
      EXPECT_TRUE(visited.at(i));
    }
  }
};

TEST_F(CircuitNeighbourhoodTest, all_values_are_initialised) {
  _assignment->initialise(_random);

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, fixed_vars_are_considered) {
  _assignment->initialise(_random);
  _assignment->performProbe(_random);
  _assignment->commitLastProbe();

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, moves_maintain_circuit) {
  static int CONFIDENCE = 1000;

  CircuitNeighbourhood neighbourhood(std::vector<SearchVar>(next), 1);

  _assignment->initialise(_random);

  auto schedule = AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(_random, *schedule, *_assignment);

  for (auto i = 0; i < CONFIDENCE; i++) {
    _random.seed(std::time(nullptr));
    neighbourhood.randomMove(_random, *_assignment);
  }
}

}  // namespace atlantis::testing
