#include "atlantis/search/neighborhoods/countNeighborhood.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {

CountNeighborhood::CountNeighborhood(std::vector<SearchVar>&& vars, Int needle,
                                     size_t amount)
    : _vars(std::move(vars)),
      _needle(needle),
      _amount(amount),
      _curTimestamp(NULL_TIMESTAMP),
      _index1(0),
      _index2(0) {
  assert(_vars.size() > 1);
  _eligibleIndices.reserve(_vars.size());
  _ineligibleIndices.reserve(_vars.size());
  for (size_t i = 0; i < _vars.size(); ++i) {
    if (_vars[i].domain()->contains(needle)) {
      _eligibleIndices.emplace_back(i);
    } else {
      _ineligibleIndices.emplace_back(i);
    }
  }
  assert(amount <= _eligibleIndices.size());
}

void CountNeighborhood::initialize(RandomProvider& random,
                                   Assignment& assignment) {
  random.shuffle(_eligibleIndices);

  for (size_t i = 0; i < _amount; ++i) {
    assignment.set(_vars[_eligibleIndices[i]].solverId(), _needle);
  }
  for (size_t i = _amount; i < _eligibleIndices.size(); ++i) {
    const auto eligibleIndex = _eligibleIndices[i];
    assignment.set(_vars[eligibleIndex].solverId(),
                   random.inDomain(*_vars[eligibleIndex].domain(), _needle));
  }
  for (const auto index : _ineligibleIndices) {
    assignment.set(_vars[index].solverId(),
                   random.inDomain(*_vars[index].domain()));
  }
}

size_t CountNeighborhood::randomMove(RandomProvider& random,
                                     Assignment& assignment) {
  if (_eligibleIndices.size() <= _amount) {
    return 0;
  }
  _index1 = random.intInRange(0, _amount - 1);
  _index2 = random.intInRange(_amount, _eligibleIndices.size() - 1);
  _curTimestamp = assignment.currentTimestamp();
  assignment.set(
      _vars[_eligibleIndices[_index1]].solverId(),
      random.inDomain(*_vars[_eligibleIndices[_index1]].domain(), _needle));
  assignment.set(_vars[_eligibleIndices[_index2]].solverId(), _needle);
  return 2;
}

void CountNeighborhood::commitIf(const Assignment& assignment) {
  if (assignment.currentTimestamp() == _curTimestamp) {
    assert(_index1 < _amount);
    assert(_amount <= _index2);
    assert(_index2 < _eligibleIndices.size());
    std::swap(_eligibleIndices[_index1], _eligibleIndices[_index2]);
  }
}

}  // namespace atlantis::search::neighborhoods
