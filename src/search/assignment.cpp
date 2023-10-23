#include "search/assignment.hpp"

namespace atlantis::search {

Assignment::Assignment(propagation::PropagationEngine& engine,
                       propagation::VarId violation,
                       propagation::VarId objective,
                       propagation::ObjectiveDirection objectiveDirection)
    : _engine(engine),
      _violation(violation),
      _objective(objective),
      _objectiveDirection(objectiveDirection) {
  _searchVariables.reserve(engine.searchVariables().size());
  for (const propagation::VarId varId : engine.searchVariables()) {
    if (engine.lowerBound(varId) != engine.upperBound(varId)) {
      _searchVariables.push_back(varId);
    }
  }
}

Int Assignment::value(propagation::VarId var) const noexcept {
  return _engine.committedValue(var);
}

bool Assignment::satisfiesConstraints() const noexcept {
  return _engine.committedValue(_violation) == 0;
}

Cost Assignment::cost() const noexcept {
  return {_engine.committedValue(_violation),
          _engine.committedValue(_objective), _objectiveDirection};
}

}  // namespace atlantis::search