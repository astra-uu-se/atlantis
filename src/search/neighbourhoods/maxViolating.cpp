#include "search/neighbourhoods/maxViolating.hpp"

#include <random>

static Int randomValue(const PropagationEngine& engine, VarId variable) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  Int lb = engine.getLowerBound(variable);
  Int ub = engine.getUpperBound(variable);

  std::uniform_int_distribution<Int> distr(lb, ub);
  return distr(gen);
}

void search::neighbourhoods::MaxViolatingNeighbourhood::initialise(
    PropagationEngine& engine) {
  for (const auto& variable : _variables)
    engine.setValue(variable, randomValue(engine, variable));
}

search::Move search::neighbourhoods::MaxViolatingNeighbourhood::randomMove(
    PropagationEngine& engine) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  std::uniform_int_distribution<size_t> distr{0, _variables.size() - 1};
  auto var = _variables[distr(gen)];
  return {var, randomValue(engine, var)};
}
