#include "search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"

#include <algorithm>

search::neighbourhoods::AllDifferentNonUniformNeighbourhood::
    AllDifferentNonUniformNeighbourhood(
        std::vector<search::SearchVariable> variables, Int domainLb,
        Int domainUb, const Engine& engine)
    : _variables(std::move(variables)),
      _variableIndices(_variables.size()),
      _domainOffset(domainLb),
      _valueIndexToVariableIndex(domainUb - domainLb + 1, _variables.size()),
      _domains(_variables.size()),
      _inDomain(_variables.size()),
      _engine(engine) {
  assert(_variables.size() > 1);
  std::iota(_variableIndices.begin(), _variableIndices.end(), 0u);
  assert(_valueIndexToVariableIndex.size() >= _variables.size());
}

static bool bipartiteMatching(
    size_t variableIndex, const std::vector<std::vector<size_t>>& forwardArcs,
    std::vector<size_t>& matching, std::vector<bool>& visited) {
  assert(variableIndex < forwardArcs.size());
  assert(matching.size() == visited.size());
  // Try each value
  for (const size_t valueIndex : forwardArcs[variableIndex]) {
    assert(valueIndex < visited.size());
    // Has the value been visited?
    if (!visited[valueIndex]) {
      visited[valueIndex] = true;
      // If the value is not in the current matching, or if
      // there exists a matching where _variables[variableIndex] = valueIndex +
      // _domainOffset:
      assert(matching[valueIndex] <= forwardArcs.size());
      if (matching[valueIndex] == forwardArcs.size() ||
          bipartiteMatching(matching[valueIndex], forwardArcs, matching,
                            visited)) {
        matching[valueIndex] = variableIndex;
        return true;
      }
    }
  }
  return false;
}

void search::neighbourhoods::AllDifferentNonUniformNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  std::vector<std::vector<size_t>> forwardArcs(_variables.size());
  std::fill(_valueIndexToVariableIndex.begin(),
            _valueIndexToVariableIndex.end(), _variables.size());
  for (size_t i = 0; i < _variables.size(); ++i) {
    _domains[i] = _variables[i].domain().values();
    _inDomain[i] = std::vector<bool>(_valueIndexToVariableIndex.size(), false);
    for (const Int val : _domains[i]) {
      _inDomain[i][toValueIndex(val)] = true;
    }
  }

  for (size_t variableIndex = 0; variableIndex < _variables.size();
       ++variableIndex) {
    forwardArcs[variableIndex].reserve(_domains[variableIndex].size());
    for (const Int val : _domains[variableIndex]) {
      const size_t valueIndex = toValueIndex(val);
      assert(valueIndex < _valueIndexToVariableIndex.size());
      forwardArcs[variableIndex].push_back(valueIndex);
    }
    random.shuffle<size_t>(forwardArcs[variableIndex]);
  }
  random.shuffle<size_t>(_variableIndices);

  for (const size_t variableIndex : _variableIndices) {
    std::vector<bool> visited(_valueIndexToVariableIndex.size(), false);
    bipartiteMatching(variableIndex, forwardArcs, _valueIndexToVariableIndex,
                      visited);
  }
#ifndef NDEBUG
  {
    std::vector<bool> varVisited(_variables.size(), false);
    for (size_t valueIndex = 0; valueIndex < _valueIndexToVariableIndex.size();
         ++valueIndex) {
      if (isValueIndexOccupied(valueIndex)) {
        const size_t variableIndex = _valueIndexToVariableIndex.at(valueIndex);
        assert(!varVisited[variableIndex]);
        varVisited[variableIndex] = true;
        assert(inDomain(variableIndex, valueIndex));
      }
    }
    for (size_t variableIndex = 0; variableIndex < varVisited.size();
         ++variableIndex) {
      assert(varVisited.at(variableIndex));
    }
  }
#endif
  for (size_t valueIndex = 0; valueIndex < _valueIndexToVariableIndex.size();
       ++valueIndex) {
    if (isValueIndexOccupied(valueIndex)) {
      assert(_valueIndexToVariableIndex.at(valueIndex) < _variables.size());
      modifications.set(
          _variables[_valueIndexToVariableIndex[valueIndex]].engineId(),
          toValue(valueIndex));
    }
  }
}

bool search::neighbourhoods::AllDifferentNonUniformNeighbourhood::randomMove(
    RandomProvider& random, Assignment& assignment, Annealer& annealer) {
  assert(sanity(assignment));
  bool didMove = false;

  for (size_t i = 0; i < _variableIndices.size(); ++i) {
    std::swap(
        _variableIndices[i],
        _variableIndices[random.intInRange(i, _variableIndices.size() - 1)]);
    const size_t variable1Index = _variableIndices[i];
#ifndef NDEBUG
    {
      const size_t value1Index = toValueIndex(
          assignment.value(_variables.at(variable1Index).engineId()));
      assert(value1Index < _valueIndexToVariableIndex.size());
      assert(variable1Index == _valueIndexToVariableIndex.at(value1Index));
    }
#endif
    for (size_t j = 0; j < _domains[variable1Index].size(); ++j) {
      std::swap(_domains[variable1Index][j],
                _domains[variable1Index][random.intInRange(
                    j, _domains[variable1Index].size() - 1)]);
      const size_t value2Index = toValueIndex(_domains[variable1Index][j]);
      assert(inDomain(variable1Index, value2Index));

      if (_valueIndexToVariableIndex[value2Index] == variable1Index) {
        continue;
      }
      if (isValueIndexOccupied(value2Index)) {
        if (canSwap(assignment, variable1Index, value2Index)) {
          didMove =
              swapValues(assignment, annealer, variable1Index, value2Index);
        }
      } else {
        didMove =
            assignValue(assignment, annealer, variable1Index, value2Index);
      }
    }
  }
  assert(sanity(assignment));
  return didMove;
}

bool search::neighbourhoods::AllDifferentNonUniformNeighbourhood::canSwap(
    const search::Assignment& assignment, size_t variable1Index,
    size_t value2Index) const noexcept {
  // variable 1:
  assert(variable1Index < _variables.size());
  assert(_valueIndexToVariableIndex.at(toValueIndex(assignment.value(
             _variables[variable1Index].engineId()))) == variable1Index);

  // variable 2:
  assert(value2Index < _valueIndexToVariableIndex.size());
  assert(isValueIndexOccupied(value2Index));
  assert(toValue(value2Index) ==
         assignment.value(
             _variables[_valueIndexToVariableIndex[value2Index]].engineId()));

  // sanity:
  assert(inDomain(variable1Index, value2Index));
  assert(_valueIndexToVariableIndex[value2Index] <= _inDomain.size());
  assert(value2Index <
         _inDomain.at(_valueIndexToVariableIndex[value2Index]).size());
  assert(toValueIndex(assignment.value(_variables[variable1Index].engineId())) <
         _inDomain.at(_valueIndexToVariableIndex[value2Index]).size());

  return inDomain(
      _valueIndexToVariableIndex[value2Index],
      toValueIndex(assignment.value(_variables[variable1Index].engineId())));
}

bool search::neighbourhoods::AllDifferentNonUniformNeighbourhood::swapValues(
    search::Assignment& assignment, search::Annealer& annealer,
    size_t variable1Index, size_t value2Index) {
  // variable 1:
  assert(variable1Index < _variables.size());
  const VarId variable1 = _variables[variable1Index].engineId();
  const Int value1 = assignment.value(variable1);
  assert(isValueIndexOccupied(toValueIndex(value1)));
  assert(_valueIndexToVariableIndex.at(toValueIndex(value1)) == variable1Index);

  // variable 2:
  assert(isValueIndexOccupied(value2Index));
  const size_t variable2Index = _valueIndexToVariableIndex[value2Index];
  assert(toValue(value2Index) ==
         assignment.value(_variables.at(variable2Index).engineId()));

  // sanity:
  assert(variable1Index != variable2Index);
  assert(toValue(value2Index) != value1);
  assert(inDomain(variable2Index, toValueIndex(value1)));
  assert(inDomain(variable1Index, value2Index));

#ifndef NDEBUG
  const size_t value1Index = toValueIndex(value1);
  assert(variable1Index == _valueIndexToVariableIndex.at(value1Index));
  assert(variable1 ==
         _variables.at(_valueIndexToVariableIndex.at(value1Index)).engineId());
  assert(variable2Index == _valueIndexToVariableIndex.at(value2Index));
  const Int value2 = toValue(value2Index);
  const VarId variable2 = _variables.at(variable2Index).engineId();
  assert(variable2 ==
         _variables.at(_valueIndexToVariableIndex.at(value2Index)).engineId());

  assert(assignment.value(variable1) == value1);
  assert(assignment.value(variable2) == value2);
#endif
  if (maybeCommit(Move<2>({variable1, _variables[variable2Index].engineId()},
                          {toValue(value2Index), value1}),
                  assignment, annealer)) {
    _valueIndexToVariableIndex[toValueIndex(value1)] = variable2Index;
    _valueIndexToVariableIndex[value2Index] = variable1Index;
    assert(variable1Index == _valueIndexToVariableIndex.at(value2Index));
    assert(variable2Index == _valueIndexToVariableIndex.at(value1Index));
    assert(
        variable2 ==
        _variables.at(_valueIndexToVariableIndex.at(value1Index)).engineId());
    assert(
        variable1 ==
        _variables.at(_valueIndexToVariableIndex.at(value2Index)).engineId());
    assert(
        variable2 ==
        _variables.at(_valueIndexToVariableIndex.at(value1Index)).engineId());

    assert(assignment.value(variable1) == value2);
    assert(assignment.value(variable2) == value1);

    assert(_valueIndexToVariableIndex.at(value1Index) == variable2Index);
    assert(_valueIndexToVariableIndex.at(value2Index) == variable1Index);
    return true;
  }
  return false;
}

bool search::neighbourhoods::AllDifferentNonUniformNeighbourhood::assignValue(
    search::Assignment& assignment, search::Annealer& annealer,
    size_t variableIndex, size_t newValueIndex) {
  assert(newValueIndex < _valueIndexToVariableIndex.size());
  assert(_valueIndexToVariableIndex[newValueIndex] == _variables.size());
  assert(variableIndex < _variables.size());
  const VarId variable = _variables[variableIndex].engineId();
  const Int oldValue = assignment.value(variable);
  const size_t oldValueIndex = toValueIndex(oldValue);

  assert(oldValueIndex != newValueIndex);
  assert(_valueIndexToVariableIndex.at(oldValueIndex) == variableIndex);

  if (maybeCommit(Move<1>({variable}, {toValue(newValueIndex)}), assignment,
                  annealer)) {
    _valueIndexToVariableIndex[newValueIndex] = variableIndex;
    _valueIndexToVariableIndex[oldValueIndex] = _variables.size();
    return true;
  }

  return false;
}
