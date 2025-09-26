#include "atlantis/search/objective.hpp"

#include <fznparser/model.hpp>
#include <limits>

#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"

namespace atlantis::search {

Objective::Objective(propagation::Solver& solver,
                     ObjectiveDirection problemType)
    : _solver(solver), _problemType(problemType) {}

propagation::VarViewId Objective::registerNode(
    propagation::VarViewId totalViolationVarId,
    propagation::VarViewId objectiveVarId) {
  assert(_solver.isOpen());
  _objective = objectiveVarId;
  if (_problemType == ObjectiveDirection::NONE) {
    return totalViolationVarId;
  }

  const Int initialBound = _problemType == ObjectiveDirection::MINIMIZE
                               ? _solver.upperBound(objectiveVarId)
                               : _solver.lowerBound(objectiveVarId);

  _bound = _solver.makeIntVar(initialBound, _solver.lowerBound(objectiveVarId),
                              _solver.upperBound(objectiveVarId));

  const auto boundViolation = static_cast<propagation::VarId>(
      _solver.makeIntVar(0, 0, std::numeric_limits<Int>::max()));

  if (_problemType == ObjectiveDirection::MINIMIZE) {
    _solver.makeViolationInvariant<propagation::LessEqual>(
        _solver, boundViolation, objectiveVarId, _bound);
  } else {
    assert(_problemType == ObjectiveDirection::MAXIMIZE);
    _solver.makeViolationInvariant<propagation::LessEqual>(
        _solver, boundViolation, _bound, objectiveVarId);
  }

  if (totalViolationVarId == propagation::NULL_ID) {
    _violation = boundViolation;
  } else {
    _violation = static_cast<propagation::VarId>(
        _solver.makeIntVar(0, 0, std::numeric_limits<Int>::max()));

    _solver.makeInvariant<propagation::Linear>(
        _solver, _violation,
        std::vector<propagation::VarViewId>{boundViolation,
                                            totalViolationVarId});
  }
  return _violation;
}

void Objective::tighten() {
  if (_bound == propagation::NULL_ID) {
    return;
  }

  const Int newBound =
      _problemType == ObjectiveDirection::NONE
          ? _solver.committedValue(_bound)
          : (_solver.committedValue(_objective) +
             (_problemType == ObjectiveDirection::MINIMIZE ? -1 : 1));

  _solver.beginMove();
  _solver.setValue(_bound, newBound);
  _solver.endMove();

  _solver.beginCommit();
  _solver.query(_violation);
  _solver.endCommit();
}

void Objective::tighten(const Cost& cost) {
  if (_bound == propagation::NULL_ID) {
    return;
  }

  const Int newBound =
      _problemType == ObjectiveDirection::NONE
          ? _solver.committedValue(_bound)
          : (cost.getObjective() +
             (_problemType == ObjectiveDirection::MINIMIZE ? -1 : 1));

  _solver.beginMove();
  _solver.setValue(_bound, newBound);
  _solver.endMove();

  _solver.beginCommit();
  _solver.query(_violation);
  _solver.endCommit();
}

propagation::VarViewId Objective::bound() const noexcept { return _bound; }

}  // namespace atlantis::search
