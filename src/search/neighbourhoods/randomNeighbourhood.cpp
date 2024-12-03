#include "atlantis/search/neighbourhoods/randomNeighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

RandomNeighbourhood::RandomNeighbourhood(std::vector<SearchVar>&& vars)
    : _vars(std::move(vars)) {}

void RandomNeighbourhood::initialise(RandomProvider& random,
                                     Assignment& assignment) {
  for (auto& var : _vars) {
    assignment.set(var.solverId(), random.inDomain(var.domain()));
  }
}

bool RandomNeighbourhood::randomMove(RandomProvider& random,
                                     Assignment& assignment,
                                     Annealer& annealer) {
  auto var = random.element(_vars);

  return maybeCommit(Move(std::vector<std::pair<propagation::VarId, Int>>{
                         {var.solverId(), random.inDomain(var.domain())}}),
                     assignment, annealer);
}

}  // namespace atlantis::search::neighbourhoods
