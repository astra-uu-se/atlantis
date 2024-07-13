#include "atlantis/search/neighbourhoods/intLinEqNeighbourhood.hpp"

#include <algorithm>
#include <cassert>

namespace atlantis::search::neighbourhoods {

IntLinEqNeighbourhood::IntLinEqNeighbourhood(
    std::vector<Int>&& coeffs, std::vector<SearchVar>&& vars, Int bound,
    const propagation::SolverBase& solver)
    : _coeffs(coeffs),
      _vars(std::move(vars)),
      _bound(bound),
      _solver(solver),
      _indices(_vars.size()) {
  assert(_vars.size() > 1);
  std::iota(_indices.begin(), _indices.end(), 0);
  assert(std::all_of(_coeffs.begin(), _coeffs.end(),
                     [](Int coeff) { return std::abs(coeff) == 1; }));
}

void IntLinEqNeighbourhood::initialise(RandomProvider& random,
                                       AssignmentModifier& modifications) {
  std::vector<std::array<Int, 2>> remainingBounds;
  remainingBounds.resize(_indices.size());
  remainingBounds.back()[0] = 0;
  remainingBounds.back()[1] = 0;
  for (Int i = static_cast<Int>(_indices.size()) - 2; i >= 0; --i) {
    const Int val1 =
        _coeffs[_indices[i]] * _vars[_indices[i]].domain().lowerBound();
    const Int val2 =
        _coeffs[_indices[i]] * _vars[_indices[i]].domain().upperBound();

    remainingBounds[_indices[i]][0] =
        remainingBounds[_indices[i + 1]][0] + std::min(val1, val2);
    remainingBounds[_indices[i]][1] =
        remainingBounds[_indices[i + 1]][1] + std::max(val1, val2);
  }

  Int remaining = -_bound;
  for (size_t i = 0; i < _indices.size(); ++i) {
    const size_t index = _indices[i];
    if (i == _indices.size() - 1) {
      remaining = (0 - remaining) / _coeffs[_indices[i]];
      assert(_vars[index].domain().lowerBound() <= remaining);
      assert(_vars[index].domain().upperBound() >= remaining);
      modifications.set(_vars[index].solverId(), remaining);
      remaining = 0;
      break;
    }

    const Int val1 = (remainingBounds[index][0] - remaining) / _coeffs[index];
    const Int val2 = (remainingBounds[index][1] - remaining) / _coeffs[index];
    const Int lb =
        std::max(_vars[index].domain().lowerBound(), std::min(val1, val2));
    const Int ub =
        std::min(_vars[index].domain().upperBound(), std::max(val1, val2));
    assert(lb <= ub);
    const Int val = random.intInRange(lb, ub);
    modifications.set(_vars[index].solverId(), val);
    remaining += _coeffs[index] * val;
    assert(remaining >= remainingBounds[index][0]);
    assert(remaining <= remainingBounds[index][1]);
  }
  assert(remaining == 0);
}

bool IntLinEqNeighbourhood::randomMove(RandomProvider& random,
                                       Assignment& assignment,
                                       Annealer& annealer) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(_indices[i],
                      _indices[random.intInRange(i, _indices.size())]);
    const size_t index1 = _indices[i];
    const Int v1 = assignment.value(_vars[index1].solverId());
    for (Int j = i + 1; j < static_cast<Int>(_indices.size()); ++j) {
      std::swap<size_t>(_indices[j],
                        _indices[random.intInRange(j, _indices.size())]);
      const size_t index2 = _indices[j];
      const Int v1 = assignment.value(_vars[index2].solverId());
      if (_coeffs[index1] == _coeffs[index2]) {
        const Int v1 = std::max(_vars[index1].domain().lowerBound(),
                                -_vars[index2].domain().upperBound());
        const Int v2 = std::min(_vars[index1].domain().upperBound(),
                                -_vars[index2].domain().lowerBound());
        if (v1 > v2) {
          continue;
        }
        assert(_vars[index1].domain().lowerBound() <= v1);
        assert(_vars[index1].domain().upperBound() >= v2);
        assert(_vars[index2].domain().lowerBound() <= -v2);
        assert(_vars[index2].domain().upperBound() >= -v1);
        const Int val = random.intInRange(v1, v2);
        return maybeCommit(
            Move<2>({_vars[index1].solverId(), _vars[index2].solverId()},
                    {val, -val}),
            assignment, annealer);
      } else {
        const Int v1 = std::max(_vars[index1].domain().lowerBound(),
                                _vars[index2].domain().lowerBound());
        const Int v2 = std::min(_vars[index1].domain().upperBound(),
                                _vars[index2].domain().upperBound());
        if (v1 > v2) {
          continue;
        }
        const Int val = random.intInRange(v1, v2);
        return maybeCommit(
            Move<2>({_vars[index1].solverId(), _vars[index2].solverId()},
                    {val, val}),
            assignment, annealer);
      }
    }
  }
  return false;
}
}  // namespace atlantis::search::neighbourhoods
