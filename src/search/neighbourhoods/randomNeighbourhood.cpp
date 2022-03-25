#include "search/neighbourhoods/randomNeighbourhood.hpp"

void search::neighbourhoods::RandomNeighbourhood::initialise(
    AssignmentModifier& modifications) {
  for (const auto& variable : _variables) {
    modifications.set(variable, randomValue(variable));
  }
}

bool search::neighbourhoods::RandomNeighbourhood::randomMove(
    Assignment& assignment, Annealer& annealer) {
  auto variable = _random.element(_variables);

  return maybeCommit(
      Move<1u>({variable}, {randomValue(variable)}),
      assignment, annealer);
}

Int search::neighbourhoods::RandomNeighbourhood::randomValue(VarId variable) {
  return _random.intInRange(_engine.lowerBound(variable),
                            _engine.upperBound(variable));
}
