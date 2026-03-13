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
      _indices(_vars.size()),
      _needle(needle),
      _amount(amount),
      _curTimestamp(NULL_TIMESTAMP),
      _index1(_vars.size()),
      _index2(_vars.size()) {
  assert(_vars.size() > 1);
  assert(amount <= _vars.size());
  assert(std::ranges::all_of(_vars, [&](const SearchVar& var) {
    return var.domain()->contains(needle);
  }));
  std::iota(_indices.begin(), _indices.end(), 0);
}

void CountNeighborhood::initialize(RandomProvider& random,
                                   Assignment& assignment) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(
        _indices[i],
        _indices[random.intInRange(i, static_cast<Int>(_indices.size()) - 1)]);
  }

  for (size_t i = 0; i < _amount; ++i) {
    assignment.set(_vars[_indices[i]].solverId(), _needle);
  }
  for (size_t i = _amount; i < _vars.size(); ++i) {
    assignment.set(_vars[_indices[i]].solverId(),
                   random.inDomain(*_vars[i].domain(), _needle));
  }
}

size_t CountNeighborhood::randomMove(RandomProvider& random,
                                     Assignment& assignment) {
  _index1 = random.intInRange(0, _amount - 1);
  _index2 = random.intInRange(_amount, _vars.size() - 1);
  _curTimestamp = assignment.currentTimestamp();
  assignment.set(_vars[_indices[_index1]].solverId(),
                 random.inDomain(*_vars[_indices[_index1]].domain(), _needle));
  assignment.set(_vars[_indices[_index2]].solverId(), _needle);
  return 2;
}

void CountNeighborhood::commitIf(const Assignment& assignment) {
  if (assignment.currentTimestamp() == _curTimestamp) {
    assert(_index1 < _amount);
    assert(_amount <= _index2);
    assert(_index2 < _vars.size());
    std::swap(_indices[_index1], _indices[_index2]);
  }
}

}  // namespace atlantis::search::neighborhoods
