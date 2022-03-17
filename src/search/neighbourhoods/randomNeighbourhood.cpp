#include "search/neighbourhoods/randomNeighbourhood.hpp"

void search::neighbourhoods::RandomNeighbourhood::initialise(
    AssignmentModification& modifications) {
  for (const auto& variable : _variables) {
    modifications.set(variable, randomValue(variable));
  }
}

std::unique_ptr<search::Move>
search::neighbourhoods::RandomNeighbourhood::randomMove() {
  // We can assume _variables is non-empty.
  auto variable = *_random.element(_variables);

  return std::make_unique<search::AssignMove>(variable, randomValue(variable));
}

Int search::neighbourhoods::RandomNeighbourhood::randomValue(VarId variable) {
  return _random.intInRange(_variableStore.lowerBound(variable),
                            _variableStore.lowerBound(variable));
}
