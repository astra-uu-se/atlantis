#include "invariants/absDiff.hpp"

#include "core/engine.hpp"
extern Id NULL_ID;

AbsDiff::AbsDiff(VarId x, VarId y, VarId absDiff)
    : Invariant(NULL_ID), _x(x), _y(y), _absDiff(absDiff) {
  _modifiedVars.reserve(1);
}

void AbsDiff::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _absDiff);
  engine.registerInvariantParameter(_id, _x, 0);
  engine.registerInvariantParameter(_id, _y, 0);
}

void AbsDiff::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _absDiff,
              std::abs(engine.getValue(ts, _x) - engine.getValue(ts, _y)));
}

void AbsDiff::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId AbsDiff::getNextParameter(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  switch (_state.getValue(ts)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void AbsDiff::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
