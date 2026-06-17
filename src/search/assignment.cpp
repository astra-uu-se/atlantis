#include "atlantis/search/assignment.hpp"

#include <utility>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"
#include "atlantis/search/savedAssignment.hpp"

namespace atlantis::search {

Assignment::Assignment(
    propagation::Solver& solver,
    std::shared_ptr<neighborhoods::Neighborhood> neighborhood,
    const propagation::VarViewId violation,
    const propagation::VarViewId objective,
    const ObjectiveDirection objectiveDirection,
    const Int objectiveOptimalValue)
    : _solver(solver),
      _neighborhood(std::move(neighborhood)),
      _violation(violation),
      _objective(objectiveDirection == ObjectiveDirection::NONE
                     ? propagation::NULL_ID
                     : objective),
      _objectiveDirection(objectiveDirection),
      _objectiveOptimalValue(objectiveOptimalValue) {
  assert(_neighborhood != nullptr);
}

Cost Assignment::initialize(RandomProvider& randomProvider) {
  _solver.beginMove();
  _neighborhood->initialize(randomProvider, *this);
  _solver.endMove();

  _solver.beginCommit();
  if (_violation != propagation::NULL_ID) {
    _solver.query(_violation);
  }
  if (_objective != propagation::NULL_ID) {
    _solver.query(_objective);
  }
  _solver.endCommit();

  return Cost{*this};
}

Cost Assignment::performProbe(RandomProvider& randomProvider) {
  _solver.beginMove();
  _neighborhood->randomMove(randomProvider, *this);
  _solver.endMove();

  _solver.beginProbe();
  if (_violation != propagation::NULL_ID) {
    _solver.query(_violation);
  }
  if (_objective != propagation::NULL_ID) {
    _solver.query(_objective);
  }
  _solver.endProbe();

  assert(_violation != propagation::NULL_ID ||
         _objective != propagation::NULL_ID);

  return Cost{*this};
}

void Assignment::commitLastProbe() {
  const Timestamp ts = _solver.currentTimestamp();

  _neighborhood->commitIf(*this);

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

std::unordered_map<propagation::VarId, Int> Assignment::currentValues() const {
  std::unordered_map<propagation::VarId, Int> saved;
  for (auto var : searchVars()) {
    saved[var] = currentValue(var);
  }

  return saved;
}

Int Assignment::committedValue(propagation::VarViewId var) const {
  return _solver.committedValue(var);
}
Int Assignment::currentViolation() const {
  return _violation == propagation::NULL_ID ? 0
                                            : _solver.currentValue(_violation);
}

Int Assignment::currentObjective() const {
  return _objective == propagation::NULL_ID ? 0
                                            : _solver.currentValue(_objective);
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

Cost Assignment::getCost() const { return Cost(*this); }

bool Assignment::hasObjective() const {
  return _objective != propagation::NULL_ID;
}

bool Assignment::hasViolation() const {
  return _violation != propagation::NULL_ID;
}

void Assignment::setAssignment(const SavedAssignment& saved) {
  const Timestamp ts = _solver.currentTimestamp();
  _solver.beginMove();
  for (auto& [varId, value] : saved.getSearchValues()) {
    set(varId, value);
  }

  for (const auto varId : searchVars()) {
    // if (_solver.hasChanged(ts, varId)) {
    _solver.setValue(varId, _solver.value(ts, varId));
    // }
  }
  _solver.endMove();

  _solver.beginCommit();
  _solver.query(_violation);
  _solver.query(_objective);
  _solver.endCommit();
}

}  // namespace atlantis::search
