#include "atlantis/search/neighbourhoods/neighbourhoodCombinator.hpp"

#include <ostream>
#include <random>
#include <typeinfo>

#include "atlantis/utils/type.hpp"

namespace atlantis::search::neighbourhoods {

NeighbourhoodCombinator::NeighbourhoodCombinator(
    std::vector<std::shared_ptr<Neighbourhood>>&& neighbourhoods)
    : _neighbourhoods(std::move(neighbourhoods)),
      _curTimestamp(NULL_TIMESTAMP),
      _curNeighbourhood(_neighbourhoods.size()) {
  assert(!_neighbourhoods.empty());
  size_t num_vars = 0;
  for (const auto& neighbourhood : _neighbourhoods) {
    num_vars += neighbourhood->coveredVars().size();
  }
  _vars.reserve(num_vars);

  std::vector<size_t> weights;
  weights.reserve(_neighbourhoods.size());
  for (const auto& neighbourhood : _neighbourhoods) {
    weights.push_back(neighbourhood->coveredVars().size());

    for (const auto& searchVar : neighbourhood->coveredVars()) {
      _vars.emplace_back(searchVar);
    }
  }

  _neighbourhoodDistribution =
      std::discrete_distribution<size_t>{weights.begin(), weights.end()};
}

void NeighbourhoodCombinator::initialise(RandomProvider& random,
                                         IAssignment& assignment) {
  for (const auto& neighbourhood : _neighbourhoods) {
    neighbourhood->initialise(random, assignment);
  }
}

size_t NeighbourhoodCombinator::randomMove(RandomProvider& random,
                                           IAssignment& assignment) {
  _curTimestamp = assignment.currentTimestamp();
  _curNeighbourhood =
      random.fromDistribution<size_t>(_neighbourhoodDistribution);
  return _neighbourhoods[_curNeighbourhood]->randomMove(random, assignment);
}

void NeighbourhoodCombinator::printNeighbourhood(logging::Logger& logger) {
  for (const auto& neighbourhood : _neighbourhoods) {
    logger.debug("Neighbourhood {} covers {} variables.",
                 demangle(typeid(neighbourhood.get()).name()),
                 neighbourhood->coveredVars().size());
  }
}

void NeighbourhoodCombinator::commitIf(const IAssignment& assignment) {
  if (_curTimestamp == assignment.currentTimestamp() &&
      _curNeighbourhood < _neighbourhoods.size()) {
    _neighbourhoods[_curNeighbourhood]->commitIf(assignment);
  }
}

}  // namespace atlantis::search::neighbourhoods
