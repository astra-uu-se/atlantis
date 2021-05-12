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

void LessEqual::init(Timestamp, Engine& engine) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(_id != NULL_ID);

  engine.registerInvariantDependsOnVar(_id, _x, LocalId(0));
  engine.registerInvariantDependsOnVar(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void LessEqual::recompute(Timestamp ts, Engine& engine) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(
      ts, engine, _violationId,
      std::max((Int)0, engine.getValue(ts, _x) - engine.getValue(ts, _y)));
}

void LessEqual::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId LessEqual::getNextDependency(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?m_x:m_y;
  switch (_state.getValue(ts)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessEqual::notifyCurrentDependencyChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void LessEqual::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
