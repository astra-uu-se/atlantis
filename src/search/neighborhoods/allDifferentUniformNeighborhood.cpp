#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"

#include <algorithm>
#include <cassert>

#include "atlantis/search/iAssignment.hpp"
#include "atlantis/search/randomProvider.hpp"
#include "atlantis/search/searchVariable.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::search::neighborhoods {

AllDifferentUniformNeighborhood::AllDifferentUniformNeighborhood(
    std::vector<SearchVar>&& vars)
    : _vars(std::move(vars)),
      _moveVarIdx(_vars.size()),
      _moveValIdx(_vars.front().domain()->size() - _vars.size()),
      _curTimestamp(NULL_TIMESTAMP) {
  assert(_vars.size() > 1);
  assert(_vars.front().domain()->size() >= _vars.size());
}

void AllDifferentUniformNeighborhood::initialize(RandomProvider& random,
                                                 IAssignment& assignment) {
  /*
  For each index in 0.._vars.size() - 1: (*_domain)[i] is the value assigned to
  _vars[i].

  For each index in _vars.size().._domain->size() - 1: (*_domain)[i] is
  a value no variable currently takes
  */
  // Each value in 0..i-1 is assigned to a variable.
  // Each value in i.._domain->size() - 1 is a free value.

  _freeVals.resize(_vars.front().domain()->size());
  for (size_t i = 0; i < _vars.front().domain()->size(); ++i) {
    _freeVals[i] = (*_vars.front().domain())[i];
  }

  for (Int i = 0; i < static_cast<Int>(_vars.size()); ++i) {
    // Retrieve a free variable at index valIndex:
    const auto valIndex = static_cast<size_t>(
        random.intInRange(0, static_cast<Int>(_freeVals.size()) - 1 - i));

    // Assign variable _vars[i] the retrieved value:
    assignment.set(_vars[i].solverId(), _freeVals[valIndex]);

    // the value assigned to _vars[i] is no longer free:
    std::swap(_freeVals[static_cast<Int>(_freeVals.size()) - 1 - i],
              _freeVals[valIndex]);
  }

  _freeVals.resize(_vars.front().domain()->size() - _vars.size());

  assert(std::ranges::all_of(
      _freeVals.begin(), _freeVals.end(), [&](const Int val) {
        return std::ranges::any_of(
            _vars.begin(), _vars.end(),
            [&](const auto& var) { return var.domain()->contains(val); });
      }));
}

size_t AllDifferentUniformNeighborhood::randomMove(RandomProvider& random,
                                                   IAssignment& assignment) {
  if (_freeVals.empty()) {
    // There are no free variables, a move consists of swapping the values of
    // two variables:
    return swapValues(random, assignment);
  }
  // A move is replacing the value of a variable with a free value:
  return assignValue(random, assignment);
}

size_t AllDifferentUniformNeighborhood::swapValues(RandomProvider& random,
                                                   IAssignment& assignment) {
  const size_t i = random.intInRange(0, static_cast<Int>(_vars.size()) - 1);
  const size_t j =
      (i + random.intInRange(1, static_cast<Int>(_vars.size()) - 1)) %
      _vars.size();

  _curTimestamp = NULL_TIMESTAMP;

  assignment.set(_vars[i].solverId(),
                 assignment.committedValue(_vars[j].solverId()));
  assignment.set(_vars[j].solverId(),
                 assignment.committedValue(_vars[i].solverId()));

  return 2;
}

size_t AllDifferentUniformNeighborhood::assignValue(RandomProvider& random,
                                                    IAssignment& assignment) {
  assert(_vars.size() < _vars.front().domain()->size());

  _moveVarIdx = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_vars.size()) - 1));

  _moveValIdx = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_freeVals.size()) - 1));

  assignment.set(_vars[_moveVarIdx].solverId(), _freeVals[_moveValIdx]);
  _curTimestamp = assignment.currentTimestamp();

  return 1;
}

void AllDifferentUniformNeighborhood::commitIf(const IAssignment& assignment) {
  if (_curTimestamp == assignment.currentTimestamp()) {
    assert(_moveVarIdx < _vars.size());
    assert(_moveValIdx < _freeVals.size());

    assert(assignment.currentValue(_vars[_moveVarIdx].solverId()) ==
           _freeVals[_moveValIdx]);

    _freeVals[_moveValIdx] =
        assignment.committedValue(_vars[_moveVarIdx].solverId());

    _curTimestamp = NULL_TIMESTAMP;
#ifndef NDEBUG
    for (size_t i = 0; i < _vars.size(); ++i) {
      for (size_t j = i + 1; j < _vars.size(); ++j) {
        assert(assignment.currentValue(_vars[i].solverId()) !=
               assignment.currentValue(_vars[j].solverId()));
        assert(assignment.committedValue(_vars[i].solverId()) !=
               assignment.committedValue(_vars[j].solverId()));
      }
    }
    for (const Int fVal : _freeVals) {
      for (const auto& var : _vars) {
        assert(assignment.currentValue(var.solverId()) != fVal);
      }
    }
#endif
  }
}

}  // namespace atlantis::search::neighborhoods
