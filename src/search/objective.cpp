#include "search/objective.hpp"

search::Objective::Objective(PropagationEngine& engine,
                             ObjectiveDirection objectiveDirection,
                             VarId violation, VarId objective, VarId bound)
    : _engine(engine),
      _objectiveDirection(objectiveDirection),
      _violation(violation),
      _objective(objective),
      _bound(bound) {}

search::Objective::Objective(PropagationEngine& engine,
                             ObjectiveDirection objectiveDirection,
                             VarId violation, VarId objective)
    : _engine(engine),
      _objectiveDirection(objectiveDirection),
      _violation(violation),
      _objective(objective),
      _bound(NULL_ID) {}

search::Objective search::Objective::createAndRegister(
    PropagationEngine& engine, ObjectiveDirection objectiveDirection,
    invariantgraph::InvariantGraphApplyResult& applicationResult) {
  return createAndRegister(engine, objectiveDirection,
                           applicationResult.totalViolations(),
                           applicationResult.objectiveVariable());
}

search::Objective search::Objective::createAndRegister(
    PropagationEngine& engine, ObjectiveDirection objectiveDirection,
    VarId totalViolations, VarId objectiveVariable) {
  if (objectiveDirection == ObjectiveDirection::NONE) {
    return Objective(engine, objectiveDirection, totalViolations,
                     objectiveVariable);
  }

  assert(engine.isOpen());

  const Int lb = engine.lowerBound(objectiveVariable);
  const Int ub = engine.upperBound(objectiveVariable);

  const VarId boundViolation =
      engine.makeIntVar(0, 0, std::numeric_limits<Int>::max());

  const VarId bound = engine.makeIntVar(
      objectiveDirection == ObjectiveDirection::MINIMIZE ? ub : lb, lb, ub);

  if (objectiveDirection == ObjectiveDirection::MINIMIZE) {
    // minimization. objective <= bound
    engine.makeConstraint<LessEqual>(boundViolation, objectiveVariable, bound);
  } else {
    assert(objectiveDirection == ObjectiveDirection::MAXIMIZE);
    // minimization. objective >= bound
    engine.makeConstraint<LessEqual>(boundViolation, bound, objectiveVariable);
  }

  const VarId violation =
      engine.makeIntVar(0, 0, std::numeric_limits<Int>::max());

  engine.makeInvariant<Linear>(
      std::vector<VarId>{boundViolation, totalViolations}, violation);

  return Objective(engine, objectiveDirection, violation, objectiveVariable,
                   bound);
}

void search::Objective::tighten() {
  if (_bound == NULL_ID) {
    return;
  }

  if (_objectiveDirection != ObjectiveDirection::NONE) {
    _engine.beginMove();
    _engine.setValue(
        _bound,
        _engine.committedValue(_objective) +
            (_objectiveDirection == ObjectiveDirection::MINIMIZE ? -1 : 1));

    _engine.endMove();
  }

  _engine.beginCommit();
  _engine.query(_violation);
  _engine.endCommit();
}

search::Assignment search::Objective::createAssignment() {
  return Assignment(_engine, _violation, _objective, _objectiveDirection);
}
