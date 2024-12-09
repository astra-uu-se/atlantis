#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

RandomNeighbourhood::RandomNeighbourhood(std::vector<SearchVar>&& vars)
    : _vars(std::move(vars)) {}

void RandomNeighbourhood::initialize(RandomProvider& random,
                                     IAssignment& assignment) {
  for (auto& var : _vars) {
    assignment.set(var.solverId(), random.inDomain(var.domain()));
  }
}

size_t RandomNeighbourhood::randomMove(RandomProvider& random,
                                       IAssignment& assignment) {
  auto var = random.element(_vars);
  assignment.set(var.solverId(), random.inDomain(var.domain()));
  return 1;
}

}  // namespace atlantis::search::neighbourhoods
