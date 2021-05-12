#include "constraints/equal.hpp"

#include "core/engine.hpp"

/**
 * Constraint a*x = b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
Equal::Equal(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Equal::init(Timestamp, Engine& e) {
  assert(_id != NULL_ID);

  e.registerInvariantDependsOnVar(_id, _x, LocalId(0));
  e.registerInvariantDependsOnVar(_id, _y, LocalId(0));
  registerDefinedVariable(e, _violationId);
}

void Equal::recompute(Timestamp t, Engine& e) {
  updateValue(t, e, _violationId,
              std::abs(e.getValue(t, _x) - e.getValue(t, _y)));
}

void Equal::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId Equal::getNextDependency(Timestamp t, Engine&) {
  _state.incValue(t, 1);
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?_x:_y;
  switch (_state.getValue(t)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Equal::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void Equal::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
