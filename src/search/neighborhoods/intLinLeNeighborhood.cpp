#include "atlantis/search/neighborhoods/intLinLeNeighborhood.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>

#include "atlantis/search/assignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/utils/domains.hpp"

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
  assert(_vars.size() > 1);
    std::iota(_indices.begin(), _indices.end(), 0);
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
  remainingLowerBound[_indices.back()] = 0;
  for (Int i = static_cast<Int>(_indices.size()) - 2; i >= 0; --i) {
    const Int val1 =
        _coeffs[_indices[i + 1]] * _vars[_indices[i + 1]].domain()->lowerBound();
    const Int val2 =
        _coeffs[_indices[i + 1]] * _vars[_indices[i + 1]].domain()->upperBound();

    remainingLowerBound[_indices[i]] =
        remainingLowerBound[_indices[i + 1]] + std::min(val1, val2);
  }
    assert(remainingLowerBound[_indices.front()] + std::min(_coeffs[_indices.front()] * _vars[_indices.front()].domain()->lowerBound(),
          _coeffs[_indices.front()] * _vars[_indices.front()].domain()->upperBound()) <= _bound);

  _curSum = 0;
  for (unsigned long index : _indices) {
      const Int rlb = remainingLowerBound[index];
    const Int remVal = (-rlb - _curSum) / _coeffs[index];
      assert(_coeffs[index] * remVal + rlb + _curSum <= _bound);
      assert(_coeffs[index] != 0);
    if (0 < _coeffs[index]) {
        const Int ub =
          std::min(_vars[index].domain()->upperBound(), remVal);
        assert(_vars[index].domain()->lowerBound() <= ub);
      const Int val = random.intInRange(_vars[index].domain()->lowerBound(), ub);
      assignment.set(_vars[index].solverId(), val);
      _curSum += _coeffs[index] * val;

        assert(_curSum + remainingLowerBound[index] <= _bound);
    } else {
      const Int lb =
          std::max(_vars[index].domain()->lowerBound(), remVal);
        assert(lb <= _vars[index].domain()->upperBound());
      const Int val = random.intInRange(lb, _vars[index].domain()->upperBound());
      assignment.set(_vars[index].solverId(), val);
      _curSum += _coeffs[index] * val;

        assert(_curSum + remainingLowerBound[index] <= _bound);
    }
  }
  assert(_curSum <= _bound);
}

size_t IntLinLeNeighborhood::randomMove(RandomProvider& random,
                                        Assignment& assignment) {
    _curTimestamp = assignment.currentTimestamp();
  for (size_t i = 0; i < _indices.size() - 1; ++i) {
      std::swap<size_t>(_indices[i], _indices[random.intInRange(i, _indices.size() - 1)]);
      _curVarIdx = _indices[i];
      const Int curVal = assignment.committedValue(_vars[_curVarIdx].solverId());
      const Int remVal = ((_bound - _curSum) / _coeffs[_curVarIdx]) + curVal;
      if (0 < _coeffs[_curVarIdx]) {
          const Int lb = _vars[_curVarIdx].domain()->lowerBound();
          const Int ub =
          std::min(_vars[_curVarIdx].domain()->upperBound(), remVal);
          assert(lb <= ub);
          if (lb == ub) {
              continue;
          }
          _curVarVal = random.intInRange(lb, ub, curVal);
          assignment.set(_vars[_curVarIdx].solverId(), _curVarVal);
          return 1;
      } else {
          const Int lb =
          std::max(_vars[_curVarIdx].domain()->lowerBound(), remVal);
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

void IntLinLeNeighborhood::commitIf(const Assignment &assignment) {
    if (_curTimestamp != assignment.currentTimestamp()) {
        return;
    }
    _curSum -= _coeffs[_curVarIdx] * assignment.committedValue(_vars[_curVarIdx].solverId());
    _curSum += _coeffs[_curVarIdx] * _curVarVal;
    assert(_curSum <= _bound);
}

}  // namespace atlantis::search::neighborhoods
