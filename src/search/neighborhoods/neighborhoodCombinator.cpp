#include "atlantis/search/neighborhoods/neighborhoodCombinator.hpp"

#include <random>
#include <typeinfo>

#include "atlantis/logging/logger.hpp"
#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/type.hpp"

namespace atlantis::search::neighborhoods {

NeighborhoodCombinator::NeighborhoodCombinator(
    std::vector<std::shared_ptr<Neighborhood>>&& neighborhoods)
    : _neighborhoods(std::move(neighborhoods)),
      _curTimestamp(NULL_TIMESTAMP),
      _curNeighborhood(_neighborhoods.size()) {
  assert(!_neighborhoods.empty());
  size_t num_vars = 0;
  for (const auto& neighborhood : _neighborhoods) {
    num_vars += neighborhood->coveredVars().size();
  }
  _vars.reserve(num_vars);

  std::vector<size_t> weights;
  weights.reserve(_neighborhoods.size());
  for (const auto& neighborhood : _neighborhoods) {
    weights.push_back(neighborhood->coveredVars().size());

    for (const auto& searchVar : neighborhood->coveredVars()) {
      _vars.emplace_back(searchVar);
    }
  }

  _neighborhoodDistribution =
      std::discrete_distribution<size_t>{weights.begin(), weights.end()};
}

void NeighborhoodCombinator::initialize(RandomProvider& random,
                                        IAssignment& assignment) {
  for (const auto& neighborhood : _neighborhoods) {
    neighborhood->initialize(random, assignment);
  }
}

size_t NeighborhoodCombinator::randomMove(RandomProvider& random,
                                          IAssignment& assignment) {
  _curTimestamp = assignment.currentTimestamp();
  _curNeighborhood = random.fromDistribution<size_t>(_neighborhoodDistribution);
  return _neighborhoods[_curNeighborhood]->randomMove(random, assignment);
}

void NeighborhoodCombinator::printNeighborhood(logging::Logger& logger) const {
  for (const auto& neighborhood : _neighborhoods) {
    logger.debug("Neighborhood {} covers {} variables.",
                 demangle(typeid(neighborhood.get()).name()),
                 neighborhood->coveredVars().size());
  }
}

void NeighborhoodCombinator::commitIf(const IAssignment& assignment) {
  if (_curTimestamp == assignment.currentTimestamp() &&
      _curNeighborhood < _neighborhoods.size()) {
    _neighborhoods[_curNeighborhood]->commitIf(assignment);
  }
}

}  // namespace atlantis::search::neighborhoods
