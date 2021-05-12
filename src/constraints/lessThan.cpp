#include "constraints/lessThan.hpp"

#include "core/engine.hpp"

/**
 * Constraint x < y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
LessThan::LessThan(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void LessThan::init(Timestamp, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  e.registerInvariantDependsOnVar(_id, _x, LocalId(0));
  e.registerInvariantDependsOnVar(_id, _y, LocalId(0));
  registerDefinedVariable(e, _violationId);
}

void LessThan::recompute(Timestamp t, Engine& e) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(t, e, _violationId,
              std::max((Int)0, e.getValue(t, _x) - e.getValue(t, _y) + 1));
}

void LessThan::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId LessThan::getNextDependency(Timestamp t, Engine&) {
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

void LessThan::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void LessThan::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
