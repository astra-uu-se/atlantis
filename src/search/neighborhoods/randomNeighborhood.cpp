#include "atlantis/search/neighborhoods/randomNeighborhood.hpp"

namespace atlantis::search::neighborhoods {

RandomNeighborhood::RandomNeighborhood(std::vector<SearchVar>&& vars)
    : _vars(std::move(vars)) {}

void RandomNeighborhood::initialize(RandomProvider& random,
                                    IAssignment& assignment) {
  for (auto& var : _vars) {
    assignment.set(var.solverId(), random.inDomain(*var.domain()));
  }
}

size_t RandomNeighborhood::randomMove(RandomProvider& random,
                                      IAssignment& assignment) {
  assert(!_vars.empty());
  const Int index = random.intInRange(0, static_cast<Int>(_vars.size()) - 1);
  assignment.set(
      _vars[index].solverId(),
      random.inDomain(*_vars[index].domain(),
                      assignment.committedValue(_vars[index].solverId())));
  return 1;
}

}  // namespace atlantis::search::neighborhoods
