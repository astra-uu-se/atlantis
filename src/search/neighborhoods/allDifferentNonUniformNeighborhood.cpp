#include "atlantis/search/neighborhoods/allDifferentNonUniformNeighborhood.hpp"

#include <algorithm>
#include <numeric>

#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {

AllDifferentNonUniformNeighborhood::AllDifferentNonUniformNeighborhood(
    std::vector<SearchVar>&& vars, Int domainLb, Int domainUb)
    : _vars(std::move(vars)),
      _varIndices(_vars.size()),
      _domainOffset(domainLb),
      _valueIndexToVarIndex(domainUb - domainLb + 1, _vars.size()),
      _inDomain(_vars.size()),
      _curTimestamp(NULL_TIMESTAMP),
      _moveValueIndex({0, 0}) {
  assert(_vars.size() > 1);
  std::iota(_varIndices.begin(), _varIndices.end(), 0u);
  assert(_valueIndexToVarIndex.size() >= _vars.size());
  const size_t maxDomSize =
      std::ranges::max_element(_vars.begin(), _vars.end(),
                               [&](const auto& a, const auto& b) {
                                 return a.domain()->size() < b.domain()->size();
                               })
          ->domain()
          ->size();
  _domIndices.reserve(maxDomSize);
  for (size_t i = 0; i < maxDomSize; ++i) {
    _domIndices.emplace_back(NULL_TIMESTAMP, i);
  }
}

static bool bipartiteMatching(
    size_t varIndex, const std::vector<std::vector<size_t>>& forwardArcs,
    std::vector<size_t>& matching, std::vector<bool>& visited) {
  assert(varIndex < forwardArcs.size());
  assert(matching.size() == visited.size());
  // Try each committedValue
  for (const size_t valueIndex : forwardArcs[varIndex]) {
    assert(valueIndex < visited.size());
    // Has the committedValue been visited?
    if (!visited[valueIndex]) {
      visited[valueIndex] = true;
      // If the committedValue is not in the current matching, or if
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

void AllDifferentNonUniformNeighborhood::initialize(RandomProvider& random,
                                                    IAssignment& assignment) {
  std::vector<std::vector<size_t>> forwardArcs(_vars.size());
  std::ranges::fill(_valueIndexToVarIndex.begin(), _valueIndexToVarIndex.end(),
                    _vars.size());
  for (size_t varIndex = 0; varIndex < _vars.size(); ++varIndex) {
    _inDomain[varIndex] =
        std::vector<bool>(_valueIndexToVarIndex.size(), false);
    for (const Int val : *_vars[varIndex].domain()) {
      _inDomain[varIndex][toValueIndex(val)] = true;
    }
  }

  for (size_t varIndex = 0; varIndex < _vars.size(); ++varIndex) {
    forwardArcs[varIndex].reserve(_vars[varIndex].domain()->size());
    for (const Int val : *_vars[varIndex].domain()) {
      const size_t valueIndex = toValueIndex(val);
      assert(valueIndex < _valueIndexToVarIndex.size());
      forwardArcs[varIndex].emplace_back(valueIndex);
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
    assert(std::ranges::all_of(
        varVisited.begin(), varVisited.end(),
        [&](size_t varIndex) { return varVisited.at(varIndex); }));
  }
#endif
  for (size_t valueIndex = 0; valueIndex < _valueIndexToVarIndex.size();
       ++valueIndex) {
    if (isValueIndexOccupied(valueIndex)) {
      assert(_valueIndexToVarIndex.at(valueIndex) < _vars.size());
      assignment.set(_vars[_valueIndexToVarIndex[valueIndex]].solverId(),
                     toValue(valueIndex));
    }
  }
}

size_t AllDifferentNonUniformNeighborhood::randomMove(RandomProvider& random,
                                                      IAssignment& assignment) {
  assert(sanity(assignment, true));

  for (Int i = 0; i < static_cast<Int>(_varIndices.size()); ++i) {
    std::swap(_varIndices[i],
              _varIndices[random.intInRange(
                  i, static_cast<Int>(_varIndices.size()) - 1)]);
    const size_t var1Index = _varIndices[i];
    const Timestamp ts = _domIndices.front().tmpTimestamp() + 1;
#ifndef NDEBUG
    {
      const size_t value1Index = toValueIndex(
          assignment.committedValue(_vars.at(var1Index).solverId()));
      assert(value1Index < _valueIndexToVarIndex.size());
      assert(var1Index == _valueIndexToVarIndex.at(value1Index));
    }
#endif
    for (Int j = 0; j < static_cast<Int>(_vars[var1Index].domain()->size());
         ++j) {
      // perform swap:
      const Int k = random.intInRange(
          j, static_cast<Int>(_vars[var1Index].domain()->size()) - 1);
      const Int domIndex = _domIndices[k].value(ts);
      _domIndices[k].setValue(ts, _domIndices[j].value(ts));
      _domIndices[j].setValue(ts, domIndex);

      const Int value2 = (*_vars[var1Index].domain())[domIndex];

      const size_t value2Index = toValueIndex(value2);

      assert(inDomain(var1Index, value2Index));

      if (_valueIndexToVarIndex[value2Index] == var1Index) {
        continue;
      }
      if (isValueIndexOccupied(value2Index)) {
        if (canSwap(assignment, var1Index, value2Index)) {
          return swapValues(assignment, var1Index, value2Index);
        }
      } else {
        return assignValue(assignment, var1Index, value2Index);
      }
    }
  }
  assert(sanity(assignment, false));
  return 0;
}

bool AllDifferentNonUniformNeighborhood::canSwap(
    const IAssignment& assignment, size_t var1Index,
    size_t value2Index) const noexcept {
  // var 1:
  assert(var1Index < _vars.size());
  assert(_valueIndexToVarIndex.at(toValueIndex(assignment.committedValue(
             _vars[var1Index].solverId()))) == var1Index);

  // var 2:
  assert(value2Index < _valueIndexToVarIndex.size());
  assert(isValueIndexOccupied(value2Index));
  assert(toValue(value2Index) ==
         assignment.committedValue(
             _vars[_valueIndexToVarIndex[value2Index]].solverId()));

  // sanity:
  assert(inDomain(var1Index, value2Index));
  assert(_valueIndexToVarIndex[value2Index] <= _inDomain.size());
  assert(value2Index < _inDomain.at(_valueIndexToVarIndex[value2Index]).size());
  assert(toValueIndex(assignment.committedValue(_vars[var1Index].solverId())) <
         _inDomain.at(_valueIndexToVarIndex[value2Index]).size());

  return inDomain(
      _valueIndexToVarIndex[value2Index],
      toValueIndex(assignment.committedValue(_vars[var1Index].solverId())));
}

size_t AllDifferentNonUniformNeighborhood::swapValues(IAssignment& assignment,
                                                      size_t var1Index,
                                                      size_t value2Index) {
  // var 1:
  assert(var1Index < _vars.size());
  const auto var1 = _vars[var1Index].solverId();
  const size_t value1Index = toValueIndex(assignment.committedValue(var1));
  assert(value1Index != value2Index);
  assert(isValueIndexOccupied(value1Index));
  assert(_valueIndexToVarIndex.at(value1Index) == var1Index);

  // var 2:
  assert(isValueIndexOccupied(value2Index));
  const size_t var2Index = _valueIndexToVarIndex[value2Index];
  assert(toValue(value2Index) ==
         assignment.committedValue(_vars.at(var2Index).solverId()));

  // sanity:
  assert(var1Index != var2Index);
  assert(toValue(value1Index) != toValue(value2Index));
  assert(inDomain(var2Index, value1Index));
  assert(inDomain(var1Index, value2Index));

  assert(assignment.committedValue(_vars.at(var2Index).solverId()) ==
         toValue(value2Index));

  _moveValueIndex[0] = value1Index;
  _moveValueIndex[1] = value2Index;
  _curTimestamp = assignment.currentTimestamp();

  assignment.set(var1, toValue(value2Index));
  assignment.set(_vars[var2Index].solverId(), toValue(value1Index));
  return 2;
}

size_t AllDifferentNonUniformNeighborhood::assignValue(IAssignment& assignment,
                                                       size_t varIndex,
                                                       size_t newValueIndex) {
  assert(newValueIndex < _valueIndexToVarIndex.size());
  assert(_valueIndexToVarIndex[newValueIndex] == _vars.size());
  assert(varIndex < _vars.size());
  const auto var = _vars[varIndex].solverId();
  const size_t oldValueIndex = toValueIndex(assignment.committedValue(var));

  assert(oldValueIndex != newValueIndex);
  assert(_valueIndexToVarIndex.at(oldValueIndex) == varIndex);

  _moveValueIndex[0] = oldValueIndex;
  _moveValueIndex[1] = newValueIndex;
  _curTimestamp = assignment.currentTimestamp();
  assignment.set(var, toValue(newValueIndex));

  return 1;
}

void AllDifferentNonUniformNeighborhood::commitIf(
    const IAssignment& assignment) {
  if (assignment.currentTimestamp() != _curTimestamp) {
    return;
  }

#ifndef NDEBUG

  assert(_moveValueIndex[0] < _valueIndexToVarIndex.size());
  assert(_moveValueIndex[1] < _valueIndexToVarIndex.size());

  assert(_valueIndexToVarIndex.at(_moveValueIndex[0]) < _vars.size());
  const size_t var1Index = _valueIndexToVarIndex[_moveValueIndex[0]];
  const auto var1 = _vars.at(var1Index).solverId();

  if (_valueIndexToVarIndex[_moveValueIndex[1]] < _vars.size()) {
    const size_t var2Index = _valueIndexToVarIndex[_moveValueIndex[1]];
    const auto var2 = _vars.at(var2Index).solverId();

    assert(assignment.currentValue(var1) == assignment.committedValue(var2));
    assert(assignment.committedValue(var1) == assignment.currentValue(var2));
  } else {
    assert(_valueIndexToVarIndex.at(_moveValueIndex[1]) == _vars.size());

    assert(assignment.committedValue(var1) == toValue(_moveValueIndex[0]));
    assert(assignment.currentValue(var1) == toValue(_moveValueIndex[1]));
  }

#endif

  std::swap(_valueIndexToVarIndex[_moveValueIndex[0]],
            _valueIndexToVarIndex[_moveValueIndex[1]]);

  _curTimestamp = NULL_TIMESTAMP;
}

}  // namespace atlantis::search::neighborhoods
