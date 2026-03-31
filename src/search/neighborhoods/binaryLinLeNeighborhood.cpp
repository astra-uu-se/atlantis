#include "atlantis/search/neighborhoods/binaryLinLeNeighborhood.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::search::neighborhoods {

template <bool Violation>
static Int toInt(const bool b) {
  if (Violation) {
    return b ? 0 : 1;
  } else {
    return b ? 1 : 0;
  }
}

template <bool Violation>
static bool toBool(const Int val) {
  if constexpr (Violation) {
    return val == 0;
  } else {
    return val == 1;
  }
}

template <bool Violation>
BinaryLinLeNeighborhood<Violation>::BinaryLinLeNeighborhood(std::vector<Int>&& coeffs,
                                             std::vector<SearchVar>&& vars,
                                             const Int bound)
    : _coeffs(coeffs),
      _vars(std::move(vars)),
      _indices(_vars.size()),
      _bound(bound),
      _curSum(0),
      _curTimestamp(NULL_TIMESTAMP),
      _curVarIdx(_vars.size()) {
  assert(_vars.size() > 1);
  std::iota(_indices.begin(), _indices.end(), 0);
}

template <bool Violation>
void BinaryLinLeNeighborhood<Violation>::initialize(RandomProvider& random,
                                       Assignment& assignment) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(
        _indices[i],
        _indices[random.intInRange(i, static_cast<Int>(_indices.size()) - 1)]);
  }

  std::vector<Int> remainingLowerBound(_indices.size());
  remainingLowerBound[_indices.back()] = -_bound;
  for (Int i = static_cast<Int>(_indices.size()) - 2; i >= 0; --i) {
    if (add_overflow(remainingLowerBound[_indices[i + 1]], std::min(_coeffs[_indices[i + 1]], Int{0}), remainingLowerBound[_indices[i]])) {
      remainingLowerBound[_indices[i]] = std::numeric_limits<Int>::min();
    }
  }
  assert(remainingLowerBound[_indices.front()] +
             std::min(_coeffs[_indices.front()], Int{0}) <=
         0);

  _curSum = 0;
  for (const size_t index : _indices) {
    const Int rlb = remainingLowerBound[index];
    bool included;
    if (0 < _coeffs[index]) {
      const bool mustBeFalse = _curSum + rlb + _coeffs[index] > 0;
      included = mustBeFalse ? false : random.boolean();
    } else {
      const bool mustBeTrue = _curSum + rlb > 0;
      included = mustBeTrue ? true : random.boolean();
    }
    assignment.set(_vars[index].solverId(), toInt<Violation>(included));
    _curSum += included ? _coeffs[index] : 0;
    assert(_curSum + remainingLowerBound[index] <= 0);
  }
  assert(_curSum <= _bound);
}

template <bool Violation>
size_t BinaryLinLeNeighborhood<Violation>::randomMove(RandomProvider& random,
                                         Assignment& assignment) {
  _curTimestamp = assignment.currentTimestamp();
  for (size_t i = 0; i < _indices.size(); ++i) {
    std::swap<size_t>(_indices[i],
                      _indices[random.intInRange(static_cast<Int>(i), static_cast<Int>(_indices.size()) - 1)]);
    _curVarIdx = _indices[i];
    const Int curVal = assignment.committedValue(_vars[_curVarIdx].solverId());
    Int prod;
    if (mul_overflow<Int>(_coeffs[_curVarIdx], toBool<Violation>(curVal) ? -1 : 1, prod)) {
      continue;
    }
    Int sum;
    if (add_overflow(_curSum, prod, sum)) {
      continue;
    }
    if (sum > _bound) {
      continue;
    }
    assignment.set(_vars[_curVarIdx].solverId(), 1 - curVal);
    return 1;
  }
  _curTimestamp = NULL_TIMESTAMP;
  return 0;
}

template <bool Violation>
void BinaryLinLeNeighborhood<Violation>::commitIf(const Assignment& assignment) {
  if (_curTimestamp != assignment.currentTimestamp()) {
    return;
  }
  assert(assignment.committedValue(_vars[_curVarIdx].solverId()) !=
         assignment.currentValue(_vars[_curVarIdx].solverId()));
  _curSum +=
      (toBool<Violation>(assignment.currentValue(_vars[_curVarIdx].solverId())) ? 1 : -1) *
      _coeffs[_curVarIdx];
  assert(_curSum <= _bound);
}

}  // namespace atlantis::search::neighborhoods
