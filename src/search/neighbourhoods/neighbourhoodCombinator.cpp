#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

#include <random>

void search::neighbourhoods::NeighbourhoodCombinator::initialize(
    PropagationEngine& engine) {
  for (const auto& neighbourhood : _neighbourhoods)
    neighbourhood->initialize(engine);
}

search::Move search::neighbourhoods::NeighbourhoodCombinator::randomMove(
    PropagationEngine& engine) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  std::uniform_int_distribution<size_t> distr(_neighbourhoods.size());
  return _neighbourhoods[distr(gen)]->randomMove(engine);
}
