#include "search/neighbourhoods/allDifferentNonUniformNeighbourhood.hpp"

namespace atlantis::search::neighbourhoods {

AllDifferentNonUniformNeighbourhood::AllDifferentNonUniformNeighbourhood(
    std::vector<SearchVar>&& vars, Int domainLb, Int domainUb,
    const propagation::SolverBase& solver)
    : _vars(std::move(vars)),
      _varIndices(_vars.size()),
      _domainOffset(domainLb),
      _valueIndexToVarIndex(domainUb - domainLb + 1, _vars.size()),
      _domains(_vars.size()),
      _inDomain(_vars.size()),
      _solver(solver) {
  assert(_vars.size() > 1);
  std::iota(_varIndices.begin(), _varIndices.end(), 0u);
  assert(_valueIndexToVarIndex.size() >= _vars.size());
}

static bool bipartiteMatching(
    size_t varIndex, const std::vector<std::vector<size_t>>& forwardArcs,
    std::vector<size_t>& matching, std::vector<bool>& visited) {
  assert(varIndex < forwardArcs.size());
  assert(matching.size() == visited.size());
  // Try each value
  for (const size_t valueIndex : forwardArcs[varIndex]) {
    assert(valueIndex < visited.size());
    // Has the value been visited?
    if (!visited[valueIndex]) {
      visited[valueIndex] = true;
      // If the value is not in the current matching, or if
      // there exists a matching where _vars[varIndex] = valueIndex +
      // _domainOffset:
      assert(matching[valueIndex] <= forwardArcs.size());
      if (matching[valueIndex] == forwardArcs.size() ||
          bipartiteMatching(matching[valueIndex], forwardArcs, matching,
                            visited)) {
        matching[valueIndex] = varIndex;
        return true;
      }
    }
  }
  return false;
}

void AllDifferentNonUniformNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
  std::vector<std::vector<size_t>> forwardArcs(_vars.size());
  std::fill(_valueIndexToVarIndex.begin(), _valueIndexToVarIndex.end(),
            _vars.size());
  for (size_t i = 0; i < _vars.size(); ++i) {
    _domains[i] = _vars[i].domain().values();
    _inDomain[i] = std::vector<bool>(_valueIndexToVarIndex.size(), false);
    for (const Int val : _domains[i]) {
      _inDomain[i][toValueIndex(val)] = true;
    }
  }

  for (size_t varIndex = 0; varIndex < _vars.size(); ++varIndex) {
    forwardArcs[varIndex].reserve(_domains[varIndex].size());
    for (const Int val : _domains[varIndex]) {
      const size_t valueIndex = toValueIndex(val);
      assert(valueIndex < _valueIndexToVarIndex.size());
      forwardArcs[varIndex].push_back(valueIndex);
    }
    random.shuffle<size_t>(forwardArcs[varIndex]);
  }
  random.shuffle<size_t>(_varIndices);

  for (const size_t varIndex : _varIndices) {
    std::vector<bool> visited(_valueIndexToVarIndex.size(), false);
    bipartiteMatching(varIndex, forwardArcs, _valueIndexToVarIndex, visited);
  }
#ifndef NDEBUG
  {
    std::vector<bool> varVisited(_vars.size(), false);
    for (size_t valueIndex = 0; valueIndex < _valueIndexToVarIndex.size();
         ++valueIndex) {
      if (isValueIndexOccupied(valueIndex)) {
        const size_t varIndex = _valueIndexToVarIndex.at(valueIndex);
        assert(!varVisited[varIndex]);
        varVisited[varIndex] = true;
        assert(inDomain(varIndex, valueIndex));
      }
    }
    assert(std::all_of(varVisited.begin(), varVisited.end(), [&](size_t varIndex) {
      return varVisited.at(varIndex);
    }));
  }
#endif
  for (size_t valueIndex = 0; valueIndex < _valueIndexToVarIndex.size();
       ++valueIndex) {
    if (isValueIndexOccupied(valueIndex)) {
      assert(_valueIndexToVarIndex.at(valueIndex) < _vars.size());
      modifications.set(_vars[_valueIndexToVarIndex[valueIndex]].solverId(),
                        toValue(valueIndex));
    }
  }
}

bool AllDifferentNonUniformNeighbourhood::randomMove(RandomProvider& random,
                                                     Assignment& assignment,
                                                     Annealer& annealer) {
  assert(sanity(assignment));
  bool didMove = false;

  for (Int i = 0; i < static_cast<Int>(_varIndices.size()); ++i) {
    std::swap(_varIndices[i],
              _varIndices[random.intInRange(i, static_cast<Int>(_varIndices.size()) - 1)]);
    const size_t var1Index = _varIndices[i];
#ifndef NDEBUG
    {
      const size_t value1Index =
          toValueIndex(assignment.value(_vars.at(var1Index).solverId()));
      assert(value1Index < _valueIndexToVarIndex.size());
      assert(var1Index == _valueIndexToVarIndex.at(value1Index));
    }
#endif
    for (Int j = 0; j < static_cast<Int>(_domains[var1Index].size()); ++j) {
      std::swap(_domains[var1Index][j],
                _domains[var1Index]
                        [random.intInRange(j, static_cast<Int>(_domains[var1Index].size()) - 1)]);
      const size_t value2Index = toValueIndex(_domains[var1Index][j]);
      assert(inDomain(var1Index, value2Index));

      if (_valueIndexToVarIndex[value2Index] == var1Index) {
        continue;
      }
      if (isValueIndexOccupied(value2Index)) {
        if (canSwap(assignment, var1Index, value2Index)) {
          didMove = swapValues(assignment, annealer, var1Index, value2Index);
        }
      } else {
        didMove = assignValue(assignment, annealer, var1Index, value2Index);
      }
    }
  }
  assert(sanity(assignment));
  return didMove;
}

bool AllDifferentNonUniformNeighbourhood::canSwap(
    const Assignment& assignment, size_t var1Index,
    size_t value2Index) const noexcept {
  // var 1:
  assert(var1Index < _vars.size());
  assert(_valueIndexToVarIndex.at(toValueIndex(
             assignment.value(_vars[var1Index].solverId()))) == var1Index);

  // var 2:
  assert(value2Index < _valueIndexToVarIndex.size());
  assert(isValueIndexOccupied(value2Index));
  assert(
      toValue(value2Index) ==
      assignment.value(_vars[_valueIndexToVarIndex[value2Index]].solverId()));

  // sanity:
  assert(inDomain(var1Index, value2Index));
  assert(_valueIndexToVarIndex[value2Index] <= _inDomain.size());
  assert(value2Index < _inDomain.at(_valueIndexToVarIndex[value2Index]).size());
  assert(toValueIndex(assignment.value(_vars[var1Index].solverId())) <
         _inDomain.at(_valueIndexToVarIndex[value2Index]).size());

  return inDomain(_valueIndexToVarIndex[value2Index],
                  toValueIndex(assignment.value(_vars[var1Index].solverId())));
}

bool AllDifferentNonUniformNeighbourhood::swapValues(Assignment& assignment,
                                                     Annealer& annealer,
                                                     size_t var1Index,
                                                     size_t value2Index) {
  // var 1:
  assert(var1Index < _vars.size());
  const propagation::VarId var1 = _vars[var1Index].solverId();
  const Int value1 = assignment.value(var1);
  assert(isValueIndexOccupied(toValueIndex(value1)));
  assert(_valueIndexToVarIndex.at(toValueIndex(value1)) == var1Index);

  // var 2:
  assert(isValueIndexOccupied(value2Index));
  const size_t var2Index = _valueIndexToVarIndex[value2Index];
  assert(toValue(value2Index) ==
         assignment.value(_vars.at(var2Index).solverId()));

  // sanity:
  assert(var1Index != var2Index);
  assert(toValue(value2Index) != value1);
  assert(inDomain(var2Index, toValueIndex(value1)));
  assert(inDomain(var1Index, value2Index));

#ifndef NDEBUG
  const size_t value1Index = toValueIndex(value1);
  assert(var1Index == _valueIndexToVarIndex.at(value1Index));
  assert(var1 == _vars.at(_valueIndexToVarIndex.at(value1Index)).solverId());
  assert(var2Index == _valueIndexToVarIndex.at(value2Index));
  const Int value2 = toValue(value2Index);
  const propagation::VarId var2 = _vars.at(var2Index).solverId();
  assert(var2 == _vars.at(_valueIndexToVarIndex.at(value2Index)).solverId());

  assert(assignment.value(var1) == value1);
  assert(assignment.value(var2) == value2);
#endif
  if (maybeCommit(Move<2>({var1, _vars[var2Index].solverId()},
                          {toValue(value2Index), value1}),
                  assignment, annealer)) {
    _valueIndexToVarIndex[toValueIndex(value1)] = var2Index;
    _valueIndexToVarIndex[value2Index] = var1Index;
    assert(var1Index == _valueIndexToVarIndex.at(value2Index));
    assert(var2Index == _valueIndexToVarIndex.at(value1Index));
    assert(var2 == _vars.at(_valueIndexToVarIndex.at(value1Index)).solverId());
    assert(var1 == _vars.at(_valueIndexToVarIndex.at(value2Index)).solverId());
    assert(var2 == _vars.at(_valueIndexToVarIndex.at(value1Index)).solverId());

    assert(assignment.value(var1) == value2);
    assert(assignment.value(var2) == value1);

    assert(_valueIndexToVarIndex.at(value1Index) == var2Index);
    assert(_valueIndexToVarIndex.at(value2Index) == var1Index);
    return true;
  }
  return false;
}

bool AllDifferentNonUniformNeighbourhood::assignValue(Assignment& assignment,
                                                      Annealer& annealer,
                                                      size_t varIndex,
                                                      size_t newValueIndex) {
  assert(newValueIndex < _valueIndexToVarIndex.size());
  assert(_valueIndexToVarIndex[newValueIndex] == _vars.size());
  assert(varIndex < _vars.size());
  const propagation::VarId var = _vars[varIndex].solverId();
  const Int oldValue = assignment.value(var);
  const size_t oldValueIndex = toValueIndex(oldValue);

  assert(oldValueIndex != newValueIndex);
  assert(_valueIndexToVarIndex.at(oldValueIndex) == varIndex);

  if (maybeCommit(Move<1>({var}, {toValue(newValueIndex)}), assignment,
                  annealer)) {
    _valueIndexToVarIndex[newValueIndex] = varIndex;
    _valueIndexToVarIndex[oldValueIndex] = _vars.size();
    return true;
  }

  return false;
}

}  // namespace atlantis::search::neighbourhoods