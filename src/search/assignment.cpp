#include "atlantis/search/assignment.hpp"

#include <functional>

namespace atlantis::search {

Assignment::Assignment(propagation::Solver& solver,
                       propagation::VarViewId violation,
                       propagation::VarViewId objective,
                       propagation::ObjectiveDirection objectiveDirection,
                       Int objectiveOptimalValue)
    : _solver(solver),
      _violation(violation),
      _objective(objective),
      _objectiveDirection(objectiveDirection),
      _objectiveOptimalValue(objectiveOptimalValue) {}

Int Assignment::value(propagation::VarViewId var) const noexcept {
  return _solver.committedValue(var);
}

bool Assignment::satisfiesConstraints() const noexcept {
  return _solver.committedValue(_violation) == 0;
}

bool Assignment::objectiveIsOptimal() const noexcept {
  return _objective == propagation::NULL_ID ||
         _solver.committedValue(_objective) == _objectiveOptimalValue;
}

Cost Assignment::cost() const noexcept {
  return {_solver.committedValue(_violation),
          _solver.committedValue(_objective), _objectiveDirection};
}

}  // namespace atlantis::search
