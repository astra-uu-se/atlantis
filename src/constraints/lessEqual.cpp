#include "constraints/lessEqual.hpp"

#include "core/engine.hpp"

/**
 * Constraint a*x <= b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
LessEqual::LessEqual(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void LessEqual::init(Timestamp, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  e.registerInvariantDependsOnVar(_id, _x, LocalId(0));
  e.registerInvariantDependsOnVar(_id, _y, LocalId(0));
  registerDefinedVariable(e, _violationId);
}

void LessEqual::recompute(Timestamp t, Engine& e) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(t, e, _violationId,
              std::max((Int)0, e.getValue(t, _x) - e.getValue(t, _y)));
}

void LessEqual::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId LessEqual::getNextDependency(Timestamp t, Engine&) {
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

void LessEqual::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void LessEqual::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
