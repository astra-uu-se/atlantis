#include <gtest/gtest.h>

#include "core/propagationEngine.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

class CircuitNeighbourhoodTest : public ::testing::Test {
 public:
  std::unique_ptr<PropagationEngine> engine;
  std::unique_ptr<search::Assignment> assignment;
  search::RandomProvider random{123456789};

  std::vector<search::SearchVariable> next;

  void SetUp() override {
    engine = std::make_unique<PropagationEngine>();

    engine->open();
    for (auto i = 0u; i < 4; ++i) {
      VarId var = engine->makeIntVar(1, 1, 4);
      next.emplace_back(var, SearchDomain(1, 4));
    }

    VarId objective = engine->makeIntVar(0, 0, 0);
    VarId violation = engine->makeIntVar(0, 0, 0);
    engine->close();

    assignment = std::make_unique<search::Assignment>(
        *engine, objective, violation, ObjectiveDirection::NONE);
  }

  void expectCycle() {
    auto count = 0;
    Int current = 0;

    do {
      current = engine->committedValue(next[current].engineId()) - 1;
      count++;
    } while (current != 0 && count <= static_cast<Int>(next.size()));

    EXPECT_EQ(count, next.size());
  }
};

class AlwaysAcceptingAnnealer : public search::Annealer {
 public:
  AlwaysAcceptingAnnealer(const search::Assignment& assignment,
                          search::RandomProvider& random)
      : Annealer(assignment, random) {}
  virtual ~AlwaysAcceptingAnnealer() = default;

 protected:
  [[nodiscard]] bool accept(Int) const override { return true; }
};

TEST_F(CircuitNeighbourhoodTest, all_values_are_initialised) {
  search::neighbourhoods::CircuitNeighbourhood neighbourhood(next);

  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, fixed_variables_are_considered) {
  next[1] = search::SearchVariable(next[1].engineId(), SearchDomain({3}));

  search::neighbourhoods::CircuitNeighbourhood neighbourhood(next);

  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  expectCycle();
}

TEST_F(CircuitNeighbourhoodTest, moves_maintain_circuit) {
  static int CONFIDENCE = 10;

  search::neighbourhoods::CircuitNeighbourhood neighbourhood(next);
  assignment->assign(
      [&](auto& modifier) { neighbourhood.initialise(random, modifier); });

  AlwaysAcceptingAnnealer annealer(*assignment, random);

  for (auto i = 0; i < CONFIDENCE; i++) {
    random.seed(std::time(nullptr));
    neighbourhood.randomMove(random, *assignment, annealer);
  }
}
