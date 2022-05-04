#include "constraints/powDomain.hpp"

#include "core/engine.hpp"

/**
 * Constraint x != 0 && y >= 0
 * Required for Pow invariants since pow(0, v) where v < 0 is undefined.
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y parameter of rhs
 */
PowDomain::PowDomain(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void PowDomain::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _x, LocalId(0));
  engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void PowDomain::updateBounds(Engine& engine, bool widenOnly) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  const Int yLb = engine.lowerBound(_y);
  const Int yUb = engine.upperBound(_y);

  const Int lb = xLb == 0 && xUb == 0 && yUb < 0 ? 1 : 0;
  const Int ub = xLb <= 0 && 0 <= xUb && yLb < 0 ? 1 : 0;

  engine.updateBounds(_violationId, lb, ub, widenOnly);
}

void PowDomain::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _violationId,
              engine.value(ts, _x) == 0 && engine.value(ts, _y) < 0 ? 1 : 0);
}

void PowDomain::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId PowDomain::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void PowDomain::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void PowDomain::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}

[[nodiscard]] bool PowDomain::shouldPost(Engine& engine, VarId x, VarId y) {
  return engine.lowerBound(x) <= 0 && 0 <= engine.upperBound(x) &&
         engine.lowerBound(y) < 0;
}
