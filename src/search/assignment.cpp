#include "atlantis/search/assignment.hpp"

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::search {

Assignment::Assignment(propagation::Solver& solver,
                       neighborhoods::Neighborhood& neighborhood,
                       propagation::VarViewId violation,
                       propagation::VarViewId objective,
                       ObjectiveDirection objectiveDirection,
                       Int objectiveOptimalValue)
    : _solver(solver),
      _neighborhood(neighborhood),
      _violation(violation),
      _objective(objective),
      _objectiveDirection(objectiveDirection),
      _objectiveOptimalValue(objectiveOptimalValue) {}

Cost Assignment::initialize(RandomProvider& randomProvider) {
  _solver.beginMove();
  _neighborhood.initialize(randomProvider, *this);
  _solver.endMove();

  _solver.beginCommit();
  if (_violation != propagation::NULL_ID) {
    _solver.query(_violation);
  }
  if (_objective != propagation::NULL_ID) {
    _solver.query(_objective);
  }
  _solver.endCommit();

  return {
      _violation == propagation::NULL_ID ? 0 : _solver.currentValue(_violation),
      _objective == propagation::NULL_ID ? 0 : _solver.currentValue(_objective),
      _objectiveDirection};
}

Cost Assignment::performProbe(RandomProvider& randomProvider) {
  _solver.beginMove();
  _neighborhood.randomMove(randomProvider, *this);
  _solver.endMove();

  _solver.beginProbe();
  if (_violation != propagation::NULL_ID) {
    _solver.query(_violation);
  }
  if (_objective != propagation::NULL_ID) {
    _solver.query(_objective);
  }
  _solver.endProbe();

  return {
      _violation == propagation::NULL_ID ? 0 : _solver.currentValue(_violation),
      _objective == propagation::NULL_ID ? 0 : _solver.currentValue(_objective),
      _objectiveDirection};
}

void Assignment::commitLastProbe() {
  const Timestamp ts = _solver.currentTimestamp();

  _neighborhood.commitIf(*this);

  _solver.beginMove();
  for (const auto varId : searchVars()) {
    if (_solver.hasChanged(ts, varId)) {
      _solver.setValue(varId, _solver.value(ts, varId));
    }
  }
  _solver.endMove();

  _solver.beginCommit();
  _solver.query(_violation);
  _solver.query(_objective);
  _solver.endCommit();
}

Int Assignment::currentValue(propagation::VarViewId var) const {
  return _solver.currentValue(var);
}

Int Assignment::committedValue(propagation::VarViewId var) const {
  return _solver.committedValue(var);
}

bool Assignment::satisfiesConstraints() const {
  return _violation == propagation::NULL_ID ||
         _solver.committedValue(_violation) == 0;
}

bool Assignment::objectiveIsOptimal() const {
  return _objective == propagation::NULL_ID ||
         _solver.committedValue(_objective) == _objectiveOptimalValue;
}

void Assignment::set(propagation::VarId searchVarId, Int val) {
  _solver.setValue(searchVarId, val);
}

const std::vector<propagation::VarId>& Assignment::searchVars() const {
  return _solver.searchVars();
}

Timestamp Assignment::currentTimestamp() const {
  return _solver.currentTimestamp();
}

ObjectiveDirection Assignment::objectiveDirection() const {
  return _objectiveDirection;
}

Cost Assignment::currentCost() const {
  return {
      _violation == propagation::NULL_ID ? 0 : _solver.currentValue(_violation),
      _objective == propagation::NULL_ID ? 0 : _solver.currentValue(_objective),
      _objectiveDirection};
}

}  // namespace atlantis::search
