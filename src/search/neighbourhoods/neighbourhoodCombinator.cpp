#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

#include <random>

void search::neighbourhoods::NeighbourhoodCombinator::initialise(
    PropagationEngine& engine) {
  for (const auto& neighbourhood : _neighbourhoods)
    neighbourhood->initialise(engine);
}

search::Move search::neighbourhoods::NeighbourhoodCombinator::randomMove(
    PropagationEngine& engine) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  std::uniform_int_distribution<size_t> distr(_neighbourhoods.size());
  return _neighbourhoods[distr(gen)]->randomMove(engine);
}
