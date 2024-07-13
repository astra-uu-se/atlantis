#include "atlantis/search/neighbourhoods/intLinEqNeighbourhood.hpp"

#include <algorithm>
#include <cassert>

namespace atlantis::search::neighbourhoods {

IntLinEqNeighbourhood::IntLinEqNeighbourhood(
    std::vector<Int>&& coeffs, std::vector<SearchVar>&& vars, Int bound,
    const propagation::SolverBase& solver)
    : _coeffs(coeffs),
      _vars(std::move(vars)),
      _offset(bound),
      _solver(solver),
      _indices(_vars.size()) {
  assert(_vars.size() > 1);
  std::iota(_indices.begin(), _indices.end(), 0);
  assert(std::all_of(_coeffs.begin(), _coeffs.end(),
                     [](Int coeff) { return std::abs(coeff) == 1; }));
}

void IntLinEqNeighbourhood::initialise(RandomProvider& random,
                                       AssignmentModifier& modifications) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(_indices[i],
                      _indices[random.intInRange(i, _indices.size() - 1)]);
  }

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

  Int curSum = _offset;
  for (size_t i = 0; i < _indices.size(); ++i) {
    const size_t index = _indices[i];
    if (i == _indices.size() - 1) {
      curSum = -curSum / _coeffs[_indices[i]];
      assert(_vars[index].domain().lowerBound() <= curSum);
      assert(_vars[index].domain().upperBound() >= curSum);
      modifications.set(_vars[index].solverId(), curSum);
      curSum = 0;
      break;
    }
    const Int val1 = (-remainingBounds[index][0] - curSum) / _coeffs[index];
    const Int val2 = (-remainingBounds[index][1] - curSum) / _coeffs[index];
    const Int lb =
        std::max(_vars[index].domain().lowerBound(), std::min(val1, val2));
    const Int ub =
        std::min(_vars[index].domain().upperBound(), std::max(val1, val2));
    assert(lb <= ub);
    const Int val = random.intInRange(lb, ub);
    modifications.set(_vars[index].solverId(), val);
    curSum += _coeffs[index] * val;
    assert(curSum >= remainingBounds[index][0]);
    assert(curSum <= remainingBounds[index][1]);
  }
  assert(curSum == 0);
}

bool IntLinEqNeighbourhood::randomMove(RandomProvider& random,
                                       Assignment& assignment,
                                       Annealer& annealer) {
  for (Int i = 0; i < static_cast<Int>(_indices.size()) - 1; ++i) {
    std::swap<size_t>(_indices[i],
                      _indices[random.intInRange(i, _indices.size() - 1)]);
    const size_t index1 = _indices[i];
    const Int cur1 = assignment.value(_vars[index1].solverId());
    const Int lb1 = _vars[index1].domain().lowerBound();
    const Int ub1 = _vars[index1].domain().upperBound();

    for (Int j = i + 1; j < static_cast<Int>(_indices.size()); ++j) {
      std::swap<size_t>(_indices[j],
                        _indices[random.intInRange(j, _indices.size() - 1)]);
      const size_t index2 = _indices[j];
      const Int cur2 = assignment.value(_vars[index2].solverId());
      const Int lb2 = _vars[index2].domain().lowerBound();
      const Int ub2 = _vars[index2].domain().upperBound();

      if (_coeffs[index1] == _coeffs[index2]) {
        const Int v1 = std::max(lb1 - cur1, -(ub2 - cur2));
        const Int v2 = std::min(ub1 - cur1, -(lb2 - cur2));
        if (v1 > v2) {
          continue;
        }
        const Int diff = random.intInRange(v1, v2);
        assert(lb1 <= cur1 + diff);
        assert(ub1 >= cur1 + diff);
        assert(lb2 <= cur2 - diff);
        assert(ub2 >= cur2 - diff);
        return maybeCommit(
            Move<2>({_vars[index1].solverId(), _vars[index2].solverId()},
                    {cur1 + diff, cur2 - diff}),
            assignment, annealer);
      } else {
        const Int v1 = std::max(lb1 - cur1, lb2 - cur2);
        const Int v2 = std::min(ub1 - cur1, ub2 - cur2);
        if (v1 > v2) {
          continue;
        }
        const Int diff = random.intInRange(v1, v2);
        assert(lb1 <= cur1 + diff);
        assert(ub1 >= cur1 + diff);
        assert(lb2 <= cur2 + diff);
        assert(ub2 >= cur2 + diff);
        return maybeCommit(
            Move<2>({_vars[index1].solverId(), _vars[index2].solverId()},
                    {cur1 + diff, cur2 + diff}),
            assignment, annealer);
      }
    }
  }
  return false;
}
}  // namespace atlantis::search::neighbourhoods
