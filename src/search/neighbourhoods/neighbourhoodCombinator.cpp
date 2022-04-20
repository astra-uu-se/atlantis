#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

#include <typeinfo>

#include "utils/type.hpp"

void search::neighbourhoods::NeighbourhoodCombinator::initialise(
    search::RandomProvider& random, search::AssignmentModifier& modifications) {
  for (const auto& neighbourhood : _neighbourhoods) {
    neighbourhood->initialise(random, modifications);
  }
}

bool search::neighbourhoods::NeighbourhoodCombinator::randomMove(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  auto& neighbourhood = random.element(_neighbourhoods);
  return neighbourhood->randomMove(random, assignment, annealer);
}

void search::neighbourhoods::NeighbourhoodCombinator::printNeighbourhood(
    std::ostream& out) {
  for (const auto& neighbourhood : _neighbourhoods) {
    out << "Neighbourhood " << demangle(typeid(*neighbourhood).name())
        << " covers " << neighbourhood->coveredVariables().size()
        << " variables." << std::endl;
  }
}
