#include "search/neighbourhoods/randomNeighbourhood.hpp"

void search::neighbourhoods::RandomNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  for (const auto& variable : _variables) {
    modifications.set(variable, randomValue(random, variable));
  }
}

bool search::neighbourhoods::RandomNeighbourhood::randomMove(
    RandomProvider& random, Assignment& assignment, Annealer& annealer) {
  auto variable = random.element(_variables);

  return maybeCommit(Move<1u>({variable}, {randomValue(random, variable)}),
                     assignment, annealer);
}

Int search::neighbourhoods::RandomNeighbourhood::randomValue(
    RandomProvider& random, VarId variable) {
  return random.intInRange(_engine.lowerBound(variable),
                           _engine.upperBound(variable));
}
