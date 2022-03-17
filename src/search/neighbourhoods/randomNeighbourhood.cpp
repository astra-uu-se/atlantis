#include "search/neighbourhoods/randomNeighbourhood.hpp"

void search::neighbourhoods::RandomNeighbourhood::initialise(
    AssignmentModification& modifications) {
  for (const auto& variable : _variables) {
    modifications.set(variable, randomValue(variable));
  }
}

search::Move<1u>
search::neighbourhoods::RandomNeighbourhood::randomMove() {
  // We can assume _variables is non-empty.
  auto variable = *_random.element(_variables);
  auto value = randomValue(variable);

  return { {variable}, {value} };
}

Int search::neighbourhoods::RandomNeighbourhood::randomValue(VarId variable) {
  return _random.intInRange(_engine.lowerBound(variable),
                            _engine.upperBound(variable));
}
