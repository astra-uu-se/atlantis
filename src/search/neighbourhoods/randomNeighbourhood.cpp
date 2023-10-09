#include "search/neighbourhoods/randomNeighbourhood.hpp"

namespace search::neighbourhoods {

void RandomNeighbourhood::initialise(RandomProvider& random,
                                     AssignmentModifier& modifications) {
  for (auto& variable : _variables) {
    modifications.set(variable.engineId(), random.inDomain(variable.domain()));
  }
}

bool RandomNeighbourhood::randomMove(RandomProvider& random,
                                     Assignment& assignment,
                                     Annealer& annealer) {
  auto variable = random.element(_variables);

  return maybeCommit(
      Move<1u>({variable.engineId()}, {random.inDomain(variable.domain())}),
      assignment, annealer);
}

}  // namespace search::neighbourhoods