#include <gtest/gtest.h>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/annealing/annealerContainer.hpp"
#include "atlantis/search/neighbourhoods/circuitNeighbourhood.hpp"

namespace atlantis::testing {

using namespace atlantis::search;

class CircuitNeighbourhoodTest : public ::testing::Test {
 public:
  std::unique_ptr<propagation::Solver> solver;
  std::unique_ptr<search::Assignment> assignment;
  search::RandomProvider random{123456789};

  std::vector<search::SearchVar> next;

  void SetUp() override {
    solver = std::make_unique<propagation::Solver>();

    solver->open();
    for (auto i = 0u; i < 4; ++i) {
      propagation::VarId var = solver->makeIntVar(1, 1, 4);
      next.emplace_back(var, SearchDomain(1, 4));
    }

    propagation::VarId objective = solver->makeIntVar(0, 0, 0);
    propagation::VarId violation = solver->makeIntVar(0, 0, 0);
    solver->close();

    assignment = std::make_unique<search::Assignment>(
        *solver, objective, violation, propagation::ObjectiveDirection::NONE);
  }

  void expectCycle() {
    auto count = 0;
    Int current = 0;

    do {
      current = solver->committedValue(next[current].solverId()) - 1;
      count++;
    } while (current != 0 && count <= static_cast<Int>(next.size()));

    EXPECT_EQ(count, next.size());
  }
};

class AlwaysAcceptingAnnealer : public search::Annealer {
 public:
  AlwaysAcceptingAnnealer(const search::Assignment& assignment,
                          search::RandomProvider& random,
                          search::AnnealingSchedule& schedule)
      : Annealer(assignment, random, schedule) {}

 protected:
  [[nodiscard]] bool accept(Int) override { return true; }
};

TEST_F(CircuitNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::CircuitNeighbourhood neighbourhood(
      (std::vector<search::SearchVar>(next)));

  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, fixed_vars_are_considered) {
  next[1] = search::SearchVar(next[1].solverId(), SearchDomain({3}));

  search::neighbourhoods::CircuitNeighbourhood neighbourhood(
      (std::vector<search::SearchVar>(next)));

  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, moves_maintain_circuit) {
  static int CONFIDENCE = 1000;

  search::neighbourhoods::CircuitNeighbourhood neighbourhood(
      (std::vector<search::SearchVar>(next)));
  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  auto schedule = search::AnnealerContainer::cooling(0.99, 4);
  AlwaysAcceptingAnnealer annealer(*assignment, random, *schedule);

  for (auto i = 0; i < CONFIDENCE; i++) {
    random.seed(std::time(nullptr));
    neighbourhood.randomMove(random, *assignment, annealer);
  }
}

}  // namespace atlantis::testing
