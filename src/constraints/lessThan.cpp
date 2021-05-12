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

void LessThan::init(Timestamp, Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  engine.registerInvariantDependsOnVar(_id, _x, LocalId(0));
  engine.registerInvariantDependsOnVar(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void LessThan::recompute(Timestamp ts, Engine& engine) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(
      ts, engine, _violationId,
      std::max((Int)0, engine.getValue(ts, _x) - engine.getValue(ts, _y) + 1));
}

void LessThan::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId LessThan::getNextDependency(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?_x:_y;
  switch (_state.getValue(ts)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessThan::notifyCurrentDependencyChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void LessThan::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
