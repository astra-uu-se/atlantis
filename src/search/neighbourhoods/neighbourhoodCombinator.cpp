#include "search/neighbourhoods/neighbourhoodCombinator.hpp"

#include <typeinfo>

#include "utils/type.hpp"

search::neighbourhoods::NeighbourhoodCombinator::NeighbourhoodCombinator(
    std::vector<std::unique_ptr<Neighbourhood>> neighbourhoods)
    : _neighbourhoods(std::move(neighbourhoods)) {
  assert(!_neighbourhoods.empty());

  std::vector<size_t> weights;
  for (const auto& neighbourhood : _neighbourhoods) {
    weights.push_back(neighbourhood->coveredVariables().size());

    _variables.insert(_variables.end(),
                      neighbourhood->coveredVariables().begin(),
                      neighbourhood->coveredVariables().end());
  }

  _neighbourhoodDistribution =
      std::discrete_distribution<size_t>{weights.begin(), weights.end()};
}

void search::neighbourhoods::NeighbourhoodCombinator::initialise(
    search::RandomProvider& random, search::AssignmentModifier& modifications) {
  for (const auto& neighbourhood : _neighbourhoods) {
    neighbourhood->initialise(random, modifications);
  }
}

bool search::neighbourhoods::NeighbourhoodCombinator::randomMove(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  auto& neighbourhood = selectNeighbourhood(random);
  return neighbourhood.randomMove(random, assignment, annealer);
}

void search::neighbourhoods::NeighbourhoodCombinator::printNeighbourhood(
    logging::Logger& logger) {
  for (const auto& neighbourhood : _neighbourhoods) {
    logger.debug("Neighbourhood {} covers {} variables.",
                 demangle(typeid(*neighbourhood).name()),
                 neighbourhood->coveredVariables().size());
  }
}

search::neighbourhoods::Neighbourhood&
search::neighbourhoods::NeighbourhoodCombinator::selectNeighbourhood(
    RandomProvider& random) {
  auto idx = random.fromDistribution<size_t>(_neighbourhoodDistribution);
  return *_neighbourhoods[idx];
}
