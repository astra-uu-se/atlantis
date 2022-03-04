#include "constraints/lessEqual.hpp"

#include "core/engine.hpp"

/**
 * Constraint x <= y
 * @param violationId id for the violationCount
 * @param x variable of lhs
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

  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void LessEqual::recompute(Timestamp ts, Engine& engine) {
  // Dereference safe as incValue does not retain ptr.
  updateValue(ts, engine, _violationId,
              std::max((Int)0, engine.value(ts, _x) - engine.value(ts, _y)));
}

void LessEqual::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId LessEqual::nextInput(Timestamp ts, Engine&) {
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?_x:_y;
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void LessEqual::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void LessEqual::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
