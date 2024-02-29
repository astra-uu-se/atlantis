#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

void RandomNeighbourhood::initialise(RandomProvider& random,
                                     AssignmentModifier& modifications) {
  for (auto& var : _vars) {
    modifications.set(var.solverId(), random.inDomain(var.domain()));
  }
}

bool RandomNeighbourhood::randomMove(RandomProvider& random,
                                     Assignment& assignment,
                                     Annealer& annealer) {
  auto var = random.element(_vars);

  return maybeCommit(
      Move<1u>({var.solverId()}, {random.inDomain(var.domain())}), assignment,
      annealer);
}

}  // namespace atlantis::search::neighbourhoods
