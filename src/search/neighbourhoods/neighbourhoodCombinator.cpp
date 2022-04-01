#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

void search::neighbourhoods::NeighbourhoodCombinator::initialise(
    search::RandomProvider& random, search::AssignmentModifier& modifications) {
  for (const auto& neighbourhood : _neighbourhoods) {
    neighbourhood->initialise(random, modifications);
  }
}

bool search::neighbourhoods::NeighbourhoodCombinator::randomMove(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  auto& neighbourhood = random.element(_neighbourhoods);
  return neighbourhood->randomMove(random, assignment, annealer);
}