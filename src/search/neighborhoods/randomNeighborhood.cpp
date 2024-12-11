#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"

namespace atlantis::search::neighborhoods {

RandomNeighborhood::RandomNeighborhood(std::vector<SearchVar>&& vars)
    : _vars(std::move(vars)) {}

void RandomNeighborhood::initialize(RandomProvider& random,
                                    IAssignment& assignment) {
  for (auto& var : _vars) {
    assignment.set(var.solverId(), random.inDomain(var.domain()));
  }
}

size_t RandomNeighborhood::randomMove(RandomProvider& random,
                                      IAssignment& assignment) {
  auto var = random.element(_vars);
  assignment.set(var.solverId(), random.inDomain(var.domain()));
  return 1;
}

}  // namespace atlantis::search::neighborhoods
