#include "constraints/notEqualConst.hpp"

#include "core/engine.hpp"

/**
 * Constraint x != y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y parameter of rhs
 */
NotEqualConst::NotEqualConst(VarId violationId, VarId x, Int y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void NotEqualConst::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void NotEqualConst::updateBounds(Engine& engine, bool widenOnly) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  if (xUb < _y || _y < xLb) {
    engine.updateBounds(_violationId, 0, 0, widenOnly);
    return;
  }

  for (const Int val : std::array<Int, 3>{xUb, _y, _y}) {
    if (xLb != val) {
      engine.updateBounds(_violationId, 0, 1, widenOnly);
      return;
    }
  }
  engine.updateBounds(_violationId, 1, 1, widenOnly);
}

void NotEqualConst::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _violationId, engine.value(ts, _x) == _y);
}

void NotEqualConst::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId NotEqualConst::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    default:
      return NULL_ID;
  }
}

void NotEqualConst::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void NotEqualConst::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

[[nodiscard]] bool NotEqualConst::shouldPost(Engine& engine, VarId x, Int y) {
  return engine.lowerBound(x) <= y && y <= engine.upperBound(x);
}
