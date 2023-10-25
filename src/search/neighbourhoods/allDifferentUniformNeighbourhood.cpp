#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

AllDifferentUniformNeighbourhood::AllDifferentUniformNeighbourhood(
    std::vector<SearchVar>&& vars, std::vector<Int> domain,
    const propagation::SolverBase& solver)
    : _vars(std::move(vars)),
      _domain(std::move(domain)),
      _solver(solver),
      _freeVars(_domain.size()) {
  assert(_vars.size() > 1);
  assert(_domain.size() >= _vars.size());

  std::sort(_domain.begin(), _domain.end());

  _domIndices.resize(_domain.size());
  _offset = _domain.front();
  for (auto i = 0u; i < _domain.size(); ++i) {
    _domIndices[_domain[i] - _offset] = i;
  }
}

void AllDifferentUniformNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  _freeVars = _domain.size();

  for (auto const& var : _vars) {
    auto idx = random.intInRange(0, static_cast<Int>(_freeVars) - 1);
    auto value = _domain[idx];
    modifications.set(var.solverId(), value);

    _domain[idx] = _domain[_freeVars - 1];
    _domain[_freeVars - 1] = value;
    --_freeVars;
  }
}

bool AllDifferentUniformNeighbourhood::randomMove(RandomProvider& random,
                                                  Assignment& assignment,
                                                  Annealer& annealer) {
  if (_freeVars == 0) {
    return swapValues(random, assignment, annealer);
  }

  return assignValue(random, assignment, annealer);
}

bool AllDifferentUniformNeighbourhood::swapValues(RandomProvider& random,
                                                  Assignment& assignment,
                                                  Annealer& annealer) {
  size_t i = random.intInRange(0, static_cast<Int>(_vars.size()) - 1);
  size_t j = (i + random.intInRange(1, static_cast<Int>(_vars.size()) - 1)) %
             _vars.size();

  auto var1 = _vars[i].solverId();
  auto var2 = _vars[j].solverId();

  Int value1 = assignment.value(var1);
  Int value2 = assignment.value(var2);

  return maybeCommit(Move<2>({var1, var2}, {value2, value1}), assignment,
                     annealer);
}

bool AllDifferentUniformNeighbourhood::assignValue(RandomProvider& random,
                                                   Assignment& assignment,
                                                   Annealer& annealer) {
  auto var = random.element(_vars).solverId();
  Int oldValue = assignment.value(var);
  size_t oldValueIdx = _domIndices[oldValue - _offset];

  Int newValueIdx = random.intInRange(0, static_cast<Int>(_freeVars) - 1);
  Int newValue = _domain[newValueIdx];

  if (maybeCommit(Move<1>({var}, {newValue}), assignment, annealer)) {
    _domain[newValueIdx] = oldValue;
    _domain[oldValueIdx] = newValue;

    _domIndices[newValue - _offset] = oldValueIdx;
    _domIndices[oldValue - _offset] = newValueIdx;

    return true;
  }

  return false;
}

}  // namespace atlantis::search::neighbourhoods