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
    _curSum(0) {
  assert(_vars.size() > 1);
    std::iota(_indices.begin(), _indices.end(), 0);
  assert(std::ranges::all_of(_coeffs.begin(), _coeffs.end(),
                             [](Int coeff) { return std::abs(coeff) == 1; }));
}

void IntLinLeNeighborhood::initialize(RandomProvider& random,
                                      Assignment& assignment) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(
        _indices[i],
        _indices[random.intInRange(i, static_cast<Int>(_indices.size()) - 1)]);
  }

  std::vector<Int> remainingBound;
  remainingBound.resize(_indices.size());
  remainingBound.back() = 0;
  for (Int i = static_cast<Int>(_indices.size()) - 2; i >= 0; --i) {
    const Int val1 =
        _coeffs[_indices[i]] * _vars[_indices[i]].domain()->lowerBound();
    const Int val2 =
        _coeffs[_indices[i]] * _vars[_indices[i]].domain()->upperBound();

    remainingBound[_indices[i]] =
        remainingBound[_indices[i + 1]] + std::max(val1, val2);
  }

  _curSum = 0;
  for (size_t i = 0; i < _indices.size(); ++i) {
    const size_t index = _indices[i];
    const Int remVal = (-remainingBound[index] - _curSum) / _coeffs[index];
    if (0 < _coeffs[index]) {
      const Int ub =
          std::min(_vars[index].domain()->upperBound(), remVal);
      const Int val = random.intInRange(_vars[index].domain()->lowerBound(), ub);
      assignment.set(_vars[index].solverId(), val);
      _curSum += _coeffs[index] * val;
    } else {
      assert(_coeffs[index] != 0);
      const Int lb =
          std::max(_vars[index].domain()->lowerBound(), remVal);
      const Int val = random.intInRange(lb, _vars[index].domain()->upperBound());
      assignment.set(_vars[index].solverId(), val);
      _curSum += _coeffs[index] * val;
    }
    assert(_curSum <= remainingBound[index]);
  }
  assert(_curSum <= 0);
}

size_t IntLinLeNeighborhood::randomMove(RandomProvider& random,
                                        Assignment& assignment) {
  for (size_t i = 0; i < _indices.size() - 1; ++i) {
      std::swap<size_t>(_indices[i], _indices[random.intInRange(i, _indices.size() - 1)]);
      const size_t index = _indices[i];
      const Int remVal = (_bound - _curSum) / _coeffs[index];
      if (0 < _coeffs[index]) {
          const Int lb = _vars[index].domain()->lowerBound();
          const Int ub =
          std::min(_vars[index].domain()->upperBound(), remVal);
          if (lb == ub) {
              continue;
          }
          const Int val = random.intInRange(lb, ub);
          assignment.set(_vars[index].solverId(), val);
          return 1;
      } else {
          assert(_coeffs[index] != 0);
          const Int lb =
          std::max(_vars[index].domain()->lowerBound(), remVal);
          const Int ub = _vars[index].domain()->upperBound();
          if (lb == ub) {
              continue;
          }
          const Int val = random.intInRange(lb, ub);
          assignment.set(_vars[index].solverId(), val);
          return 1;
      }
  }
  return 0;
}

void IntLinLeNeighborhood::commitIf(const Assignment &assignment) {
    _curSum = 0;
    for (size_t i = 0; i < _vars.size(); ++i) {
        _curSum += _coeffs[i] * assignment.committedValue(_vars[i].solverId());
    }
    assert(_curSum <= _bound);
}

}  // namespace atlantis::search::neighborhoods
