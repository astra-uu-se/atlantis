#include "atlantis/search/neighborhoods/intLinLeNeighborhood.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/domains.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::search::neighborhoods {

IntLinLeNeighborhood::IntLinLeNeighborhood(std::vector<Int>&& coeffs,
                                           std::vector<SearchVar>&& vars,
                                           Int bound)
    : _coeffs(coeffs),
      _vars(std::move(vars)),
      _indices(_vars.size()),
      _bound(bound),
      _curSum(0),
      _curTimestamp(NULL_TIMESTAMP),
      _curVarIdx(_vars.size()),
      _curVarVal(0) {
  assert(!_vars.empty());
  std::ranges::iota(_indices, 0);
}

static Int divRound(const Int nominator, const Int denominator) {
  const Int quotient = nominator / denominator;
  if (denominator < 0 && quotient * denominator != nominator) {
    return quotient + 1;
  }
  if (nominator < 0 && quotient * denominator != nominator) {
    return quotient - 1;
  }
  return quotient;
}

void IntLinLeNeighborhood::initialize(RandomProvider& random,
                                      Assignment& assignment) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(
        _indices[i],
        _indices[random.intInRange(i, static_cast<Int>(_indices.size()) - 1)]);
  }

  std::vector<Int> remainingLowerBound;
  remainingLowerBound.resize(_indices.size());
  remainingLowerBound[_indices.back()] = -_bound;
  for (Int i = static_cast<Int>(_indices.size()) - 2; i >= 0; --i) {
    const Int c = _coeffs[_indices[i + 1]];
    const Int lb = _vars[_indices[i + 1]].domain()->lowerBound();
    const Int ub = _vars[_indices[i + 1]].domain()->upperBound();
    const Int val1 = overflow::saturatingMul(c, lb);
    const Int val2 = overflow::saturatingMul(c, ub);
    remainingLowerBound[_indices[i]] = overflow::saturatingAdd(remainingLowerBound[_indices[i + 1]], std::min(val1, val2));
  }
  assert(remainingLowerBound[_indices.front()] +
             std::min(_coeffs[_indices.front()] *
                          _vars[_indices.front()].domain()->lowerBound(),
                      _coeffs[_indices.front()] *
                          _vars[_indices.front()].domain()->upperBound()) <=
         0);

  _curSum = 0;
  for (const size_t index : _indices) {
    const Int rlb = remainingLowerBound[index];
    const Int remVal = divRound(-rlb - _curSum, _coeffs[index]);
    assert(_coeffs[index] * remVal + rlb + _curSum <= 0);
    assert(_coeffs[index] != 0);
    if (0 < _coeffs[index]) {
      const Int ub = std::min(_vars[index].domain()->upperBound(), remVal);
      assert(_vars[index].domain()->lowerBound() <= ub);
      const Int val =
          random.intInRange(_vars[index].domain()->lowerBound(), ub);
      assignment.set(_vars[index].solverId(), val);
      _curSum += _coeffs[index] * val;

      assert(_curSum + remainingLowerBound[index] <= 0);
    } else {
      const Int lb = std::max(_vars[index].domain()->lowerBound(), remVal);
      assert(lb <= _vars[index].domain()->upperBound());
      const Int val =
          random.intInRange(lb, _vars[index].domain()->upperBound());
      assignment.set(_vars[index].solverId(), val);
      _curSum += _coeffs[index] * val;

      assert(_curSum + remainingLowerBound[index] <= 0);
    }
  }
  assert(_curSum <= _bound);
}

size_t IntLinLeNeighborhood::randomMove(RandomProvider& random,
                                        Assignment& assignment) {
  _curTimestamp = assignment.currentTimestamp();
  for (size_t i = 0; i < _indices.size(); ++i) {
    std::swap<size_t>(_indices[i],
                      _indices[random.intInRange(static_cast<Int>(i), static_cast<Int>(_indices.size()) - 1)]);
    _curVarIdx = _indices[i];
    const Int curVal = assignment.committedValue(_vars[_curVarIdx].solverId());
    Int diff;
    if (overflow::subOverflow(_bound, _curSum, &diff)) {
      continue;
    }
    const Int quotient = diff / _coeffs[_curVarIdx];
    Int remVal;
    if (overflow::addOverflow(curVal, quotient, &remVal)) {
      continue;
    }
    assert(_curSum + _coeffs[_curVarIdx] * (remVal - curVal) <= _bound);
    if (0 < _coeffs[_curVarIdx]) {
      const Int lb = _vars[_curVarIdx].domain()->lowerBound();
      const Int ub = std::min(_vars[_curVarIdx].domain()->upperBound(), remVal);
      assert(lb <= ub);
      if (lb == ub) {
        continue;
      }
      _curVarVal = random.intInRange(lb, ub, curVal);
      assignment.set(_vars[_curVarIdx].solverId(), _curVarVal);
      return 1;
    } else {
      const Int lb = std::max(_vars[_curVarIdx].domain()->lowerBound(), remVal);
      const Int ub = _vars[_curVarIdx].domain()->upperBound();
      assert(lb <= ub);
      if (lb == ub) {
        continue;
      }
      _curVarVal = random.intInRange(lb, ub, curVal);
      assignment.set(_vars[_curVarIdx].solverId(), _curVarVal);
      return 1;
    }
  }
  _curTimestamp = NULL_TIMESTAMP;
  return 0;
}

void IntLinLeNeighborhood::commitIf(const Assignment& assignment) {
  if (_curTimestamp != assignment.currentTimestamp()) {
    return;
  }
  assert(assignment.committedValue(_vars[_curVarIdx].solverId()) != _curVarVal);
  _curSum +=
      _coeffs[_curVarIdx] *
      (_curVarVal - assignment.committedValue(_vars[_curVarIdx].solverId()));
  assert(_curSum <= _bound);
}

}  // namespace atlantis::search::neighborhoods
