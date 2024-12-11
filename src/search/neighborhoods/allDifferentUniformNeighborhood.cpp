#include <algorithm>
#include <cassert>

#include "atlantis/search/neighborhoods/allDifferentUniformNeighborhood.hpp"

namespace atlantis::search::neighborhoods {

AllDifferentUniformNeighborhood::AllDifferentUniformNeighborhood(
    std::vector<SearchVar>&& vars, std::vector<Int>&& domain)
    : _vars(std::move(vars)),
      _domain(std::move(domain)),
      _moveVarIdx(_vars.size()),
      _moveValIdx(_domain.size()),
      _curTimestamp(NULL_TIMESTAMP),
      _hasFreeValues(_domain.size() > _vars.size()) {
  assert(_vars.size() > 1);
  assert(_domain.size() >= _vars.size());

  std::sort(_domain.begin(), _domain.end());
  _domain.erase(std::unique(_domain.begin(), _domain.end()), _domain.end());
}

void AllDifferentUniformNeighborhood::initialize(RandomProvider& random,
                                                 IAssignment& assignment) {
  /*
  For each index in 0.._vars.size() - 1: _domain[i] is the value assigned to
  _vars[i].

  For each index in _vars.size().._domain.size() - 1: _domain[i] is
  a value no variable currently takes
  */
  // Each value in 0..i-1 is assigned to a variable.
  // Each value in i.._domain.size() - 1 is a free value.
  for (size_t i = 0; i < _vars.size(); ++i) {
    // Retrieve a free variable at index valIndex:
    const size_t valIndex =
        static_cast<size_t>(random.intInRange(i, _domain.size() - 1));

    // Assign variable _vars[i] the retrieved value:
    assignment.set(_vars[i].solverId(), _domain[valIndex]);

    // the value assigned to _vars[i] is no longer free:
    std::swap(_domain[i], _domain[valIndex]);
  }
}

size_t AllDifferentUniformNeighborhood::randomMove(RandomProvider& random,
                                                   IAssignment& assignment) {
  if (_hasFreeValues) {
    // A move is replacing the value of a variable with a free value:
    return assignValue(random, assignment);
  }

  // There are no free variables, a move consists of swapping the values of
  // two variables:
  return swapValues(random, assignment);
}

size_t AllDifferentUniformNeighborhood::swapValues(RandomProvider& random,
                                                   IAssignment& assignment) {
  size_t i = random.intInRange(0, static_cast<Int>(_vars.size()) - 1);
  size_t j = (i + random.intInRange(1, static_cast<Int>(_vars.size()) - 1)) %
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
  assert(_vars.size() < _domain.size());

  _moveVarIdx = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_vars.size()) - 1));

  _moveValIdx = static_cast<size_t>(random.intInRange(
      static_cast<Int>(_vars.size()), static_cast<Int>(_domain.size()) - 1));

  assignment.set(_vars[_moveVarIdx].solverId(), _domain[_moveValIdx]);
  _curTimestamp = assignment.currentTimestamp();

  return 1;
}

void AllDifferentUniformNeighborhood::commitIf(const IAssignment& assignment) {
  if (_curTimestamp == assignment.currentTimestamp()) {
    assert(_moveVarIdx < _vars.size());
    assert(_vars.size() <= _moveValIdx);
    assert(_moveValIdx < _domain.size());

    assert(assignment.committedValue(_vars[_moveVarIdx].solverId()) ==
           _domain[_moveVarIdx]);
    assert(assignment.currentValue(_vars[_moveVarIdx].solverId()) ==
           _domain[_moveValIdx]);

    std::swap(_domain[_moveVarIdx], _domain[_moveValIdx]);
    _curTimestamp = NULL_TIMESTAMP;
#ifndef NDEBUG
    for (size_t i = 0; i < _vars.size(); ++i) {
      assert(assignment.currentValue(_vars[i].solverId()) == _domain[i]);
      for (size_t j = i + 1; j < _vars.size(); ++j) {
        assert(assignment.currentValue(_vars[i].solverId()) !=
               assignment.currentValue(_vars[j].solverId()));
      }
      for (size_t j = _vars.size(); j < _domain.size(); ++j) {
        assert(_domain[i] != _domain[j]);
      }
    }
    for (size_t i = _vars.size(); i < _domain.size(); ++i) {
      for (size_t j = 0; j < _vars.size(); ++j) {
        assert(_domain[i] != _domain[j]);
      }
    }
#endif
  }
}

}  // namespace atlantis::search::neighborhoods
