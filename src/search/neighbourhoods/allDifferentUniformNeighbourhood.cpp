#include "atlantis/search/neighbourhoods/allDifferentUniformNeighbourhood.hpp"

#include <algorithm>
#include <cassert>

namespace atlantis::search::neighbourhoods {

AllDifferentUniformNeighbourhood::AllDifferentUniformNeighbourhood(
    std::vector<SearchVar>&& vars, std::vector<Int>&& domain)
    : _vars(std::move(vars)), _domain(std::move(domain)) {
  assert(_vars.size() > 1);
  assert(_domain.size() >= _vars.size());

  std::sort(_domain.begin(), _domain.end());
  _domain.erase(std::unique(_domain.begin(), _domain.end()), _domain.end());
}

void AllDifferentUniformNeighbourhood::initialise(
    RandomProvider& random, AssignmentModifier& modifications) {
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
    modifications.set(_vars[i].solverId(), _domain[valIndex]);

    // the value assigned to _vars[i] is no longer free:
    std::swap(_domain[i], _domain[valIndex]);
  }

  if (_domain.size() == _vars.size()) {
    // There are no free variables, thus _domain is not needed:
    _domain.clear();
  }
}

bool AllDifferentUniformNeighbourhood::randomMove(RandomProvider& random,
                                                  Assignment& assignment,
                                                  Annealer& annealer) {
  if (_domain.empty()) {
    // There are no free variables, a move consists of swapping the values of
    // two variables:
    return swapValues(random, assignment, annealer);
  }

  // Otherwise, a move is replacing the value of a variable with a free value:
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
  const size_t selectedVarIndex = static_cast<size_t>(
      random.intInRange(0, static_cast<Int>(_vars.size()) - 1));

  const size_t selectedValIndex = static_cast<size_t>(random.intInRange(
      static_cast<Int>(_vars.size()), static_cast<Int>(_domain.size()) - 1));

  if (maybeCommit(Move<1>({_vars[selectedVarIndex].solverId()},
                          {_domain[selectedValIndex]}),
                  assignment, annealer)) {
    std::swap(_domain[selectedVarIndex], _domain[selectedValIndex]);
#ifndef NDEBUG
    for (size_t i = 0; i < _vars.size(); ++i) {
      assert(assignment.value(_vars[i].solverId()) == _domain[i]);
      for (size_t j = i + 1; j < _vars.size(); ++j) {
        assert(assignment.value(_vars[i].solverId()) !=
               assignment.value(_vars[j].solverId()));
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
    return true;
  }

  return false;
}

}  // namespace atlantis::search::neighbourhoods
