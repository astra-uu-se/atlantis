#include "atlantis/search/neighborhoods/binaryLinEqNeighborhood.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {

// If bound is negative, then multiply each coeff and the bound by -1
template <bool Violation>
BinaryLinEqNeighborhood<Violation>::BinaryLinEqNeighborhood(const std::vector<Int>& coeffs,
                          std::vector<SearchVar>&& vars, const Int bound)
    : _indices(coeffs.size()),
      _numNegative(std::ranges::count_if(coeffs, [bound](const Int& c) { return bound >= 0 ? c < 0 : c > 0; })),
      _numNegTrue{0},
      _numPosTrue{0},
      _bound(static_cast<size_t>(std::abs(bound))),
      _curTimestamp(NULL_TIMESTAMP),
      _index1(coeffs.size()),
      _index2(coeffs.size()) {
  std::ranges::iota(_indices, 0);
  std::ranges::sort(_indices, [bound, &coeffs](const size_t i1, const size_t i2) {
    return bound >= 0 ? coeffs[i1] < coeffs[i2] : coeffs[i1] > coeffs[i2];
  });
  _vars.reserve(coeffs.size());
  for (size_t i = 0; i < vars.size(); ++i) {
    _vars.emplace_back(vars[_indices[i]]);
  }
}

template <bool Violation>
void BinaryLinEqNeighborhood<Violation>::initialize(RandomProvider& random,
                                   Assignment& assignment) {
  std::ranges::iota(_indices, 0);
  for (size_t i = 0; i < _numNegative; ++i) {
    const Int idx = random.intInRange(static_cast<Int>(i), static_cast<Int>(_numNegative) - 1);
    std::swap(_indices[i], _indices[idx]);
  }
  for (size_t i = _numNegative; i < _vars.size(); ++i) {
    const Int idx = random.intInRange(static_cast<Int>(i), static_cast<Int>(_vars.size()) - 1);
    std::swap(_indices[i], _indices[idx]);
  }
  assert(_bound + _numNegative >= _vars.size());
  _numNegTrue = random.intInRange(0, std::min(_numNegative, _bound - (_vars.size() - _numNegative)));
  for (Int i = 0; i < _numNegTrue; ++i) {
    assignment.set(_vars[_indices[i]].solverId(), toInt(true));
  }
  _numPosTrue = _bound + _numNegTrue;
  for (Int i = 0; i < numPos; ++i) {
    assignment.set(_vars[_indices[_numNegative + i]].solverId(), toInt(true));
  }
}

template <bool Violation>
size_t BinaryLinEqNeighborhood<Violation>::randomMove(RandomProvider& random,
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

template <bool Violation>
void BinaryLinEqNeighborhood<Violation>::commitIf(const Assignment& assignment) {
  if (assignment.currentTimestamp() == _curTimestamp) {
    assert(_index1 < _amount);
    assert(_amount <= _index2);
    assert(_index2 < _eligibleIndices.size());
    std::swap(_eligibleIndices[_index1], _eligibleIndices[_index2]);
  }
}

}  // namespace atlantis::search::neighborhoods
