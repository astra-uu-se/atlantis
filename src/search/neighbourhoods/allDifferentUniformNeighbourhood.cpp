#include "search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

#include <algorithm>

search::neighbourhoods::AllDifferentUniformNeighbourhood::
    AllDifferentUniformNeighbourhood(
        std::vector<search::SearchVariable> variables, std::vector<Int> domain,
        const Engine& engine)
    : _variables(std::move(variables)),
      _domain(std::move(domain)),
      _engine(engine),
      _freeVariables(_domain.size()) {
  assert(_variables.size() > 1);
  assert(_domain.size() >= _variables.size());

  std::sort(_domain.begin(), _domain.end());

  _domIndices.resize(_domain.size());
  _offset = _domain.front();
  for (auto i = 0u; i < _domain.size(); ++i) {
    _domIndices[_domain[i] - _offset] = i;
  }
}

void search::neighbourhoods::AllDifferentUniformNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  _freeVariables = _domain.size();

  for (auto const& variable : _variables) {
    auto idx = random.intInRange(0, static_cast<Int>(_freeVariables) - 1);
    auto value = _domain[idx];
    modifications.set(variable.engineId(), value);

    _domain[idx] = _domain[_freeVariables - 1];
    _domain[_freeVariables - 1] = value;
    --_freeVariables;
  }
}

bool search::neighbourhoods::AllDifferentUniformNeighbourhood::randomMove(
    RandomProvider& random, Assignment& assignment, Annealer& annealer) {
  if (_freeVariables == 0) {
    return swapValues(random, assignment, annealer);
  }

  return assignValue(random, assignment, annealer);
}

bool search::neighbourhoods::AllDifferentUniformNeighbourhood::swapValues(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  size_t i = random.intInRange(0, static_cast<Int>(_variables.size()) - 1);
  size_t j =
      (i + random.intInRange(1, static_cast<Int>(_variables.size()) - 1)) %
      _variables.size();

  auto var1 = _variables[i].engineId();
  auto var2 = _variables[j].engineId();

  Int value1 = assignment.value(var1);
  Int value2 = assignment.value(var2);

  return maybeCommit(Move<2>({var1, var2}, {value2, value1}), assignment,
                     annealer);
}

bool search::neighbourhoods::AllDifferentUniformNeighbourhood::assignValue(
    search::RandomProvider& random, search::Assignment& assignment,
    search::Annealer& annealer) {
  auto var = random.element(_variables).engineId();
  Int oldValue = assignment.value(var);
  size_t oldValueIdx = _domIndices[oldValue - _offset];

  Int newValueIdx = random.intInRange(0, static_cast<Int>(_freeVariables) - 1);
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