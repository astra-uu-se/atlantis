#include "search/neighbourhoods/allDifferentNeighbourhood.hpp"

#include <algorithm>

search::neighbourhoods::AllDifferentNeighbourhood::AllDifferentNeighbourhood(
    std::vector<VarId> variables, std::vector<Int> domain, const Engine& engine)
    : _variables(std::move(variables)),
      _domain(std::move(domain)),
      _engine(engine),
      _freeVariables(_domain.size()) {
  assert(_variables.size() > 1);
  assert(_domain.size() >= _variables.size());

  std::sort(_domain.begin(), _domain.end());

  _domIndexes.resize(_domain.size());
  _offset = _domain.front();
  for (auto i = 0u; i < _domain.size(); ++i) {
    _domIndexes[_domain[i] - _offset] = i;
  }
}

void search::neighbourhoods::AllDifferentNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  _freeVariables = _domain.size();

  for (auto const& variable : _variables) {
    auto idx = random.intInRange(0, static_cast<Int>(_freeVariables) - 1);
    auto value = _domain[idx];
    modifications.set(variable, value);

    _domain[idx] = _domain[_freeVariables - 1];
    _domain[_freeVariables - 1] = value;
    --_freeVariables;
  }
}

bool search::neighbourhoods::AllDifferentNeighbourhood::randomMove(
    RandomProvider& random, Assignment& assignment, Annealer& annealer) {
  if (_freeVariables == 0) {
    return swapValues(random, assignment, annealer);
  }

  return assignValue(random, assignment, annealer);
}

bool search::neighbourhoods::AllDifferentNeighbourhood::swapValues(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  VarId var1 = random.element(_variables);
  VarId var2 = random.element(_variables);

  Int value1 = assignment.value(var1);
  Int value2 = assignment.value(var2);

  return maybeCommit(Move<2>({var1, var2}, {value2, value1}), assignment,
                     annealer);
}

bool search::neighbourhoods::AllDifferentNeighbourhood::assignValue(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  VarId var = random.element(_variables);
  Int oldValue = assignment.value(var);
  size_t oldValueIdx = _domIndexes[oldValue - _offset];

  Int newValueIdx = random.intInRange(0, static_cast<Int>(_freeVariables) - 1);
  Int newValue = _domain[newValueIdx];

  if (maybeCommit(Move<1>({var}, {newValue}), assignment, annealer)) {
    _domain[newValueIdx] = oldValue;
    _domain[oldValueIdx] = newValue;

    _domIndexes[newValue - _offset] = oldValueIdx;
    _domIndexes[oldValue - _offset] = newValueIdx;

    return true;
  }

  return false;
}
