#include "search/assignment.hpp"

namespace atlantis::search {

Assignment::Assignment(propagation::Solver& solver,
                       propagation::VarId violation,
                       propagation::VarId objective,
                       propagation::ObjectiveDirection objectiveDirection)
    : _solver(solver),
      _violation(violation),
      _objective(objective),
      _objectiveDirection(objectiveDirection) {
  _searchVars.reserve(solver.searchVars().size());
  for (const propagation::VarId varId : solver.searchVars()) {
    if (solver.lowerBound(varId) != solver.upperBound(varId)) {
      _searchVars.push_back(varId);
    }
  }
}

Int Assignment::value(propagation::VarId var) const noexcept {
  return _solver.committedValue(var);
}

bool Assignment::satisfiesConstraints() const noexcept {
  return _solver.committedValue(_violation) == 0;
}

Cost Assignment::cost() const noexcept {
  return {_solver.committedValue(_violation),
          _solver.committedValue(_objective), _objectiveDirection};
}

}  // namespace atlantis::search