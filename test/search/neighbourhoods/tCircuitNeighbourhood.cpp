#include <gtest/gtest.h>

#include "../testHelper.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class CircuitNeighbourhoodTest : public ::testing::Test {
 public:
  std::shared_ptr<propagation::Solver> _solver;
  std::shared_ptr<search::Assignment> _assignment;
  search::RandomProvider _random{123456789};

  std::vector<search::SearchVar> next;

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

    _assignment = std::make_shared<search::Assignment>(
        *_solver, objective, violation, propagation::ObjectiveDirection::NONE,
        0);
  }

  void expectCycle() {
    auto count = 0;
    Int current = 0;

    do {
      current = _solver->committedValue(next[current].solverId()) - 1;
      count++;
    } while (current != 0 && count <= static_cast<Int>(next.size()));

    EXPECT_EQ(count, next.size());
  }
};

TEST_F(CircuitNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::CircuitNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(next), 1);

  _assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, fixed_vars_are_considered) {
  next[1] = search::SearchVar(next[1].solverId(), SearchDomain({3}));

  search::neighbourhoods::CircuitNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(next), 1);

  _assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, moves_maintain_circuit) {
  static int CONFIDENCE = 1000;

  search::neighbourhoods::CircuitNeighbourhood neighbourhood(
      std::vector<search::SearchVar>(next), 1);
  _assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(_random, modifier); });

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*_assignment, _random, *schedule);

  for (auto i = 0; i < CONFIDENCE; i++) {
    _random.seed(std::time(nullptr));
    neighbourhood.randomMove(_random, *_assignment, annealer);
  }
}

}  // namespace atlantis::testing
